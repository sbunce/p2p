#ifndef H_CHANNEL
#define H_CHANNEL

//include
#include <boost/optional.hpp>
#include <boost/thread.hpp>
#include <logger.hpp>

//std
#include <cassert>
#include <list>
#include <set>

namespace channel{

//predecls for friend'ing
template<typename T> class future;
template<typename T> class promise;
template<typename T> class select;
template<typename T> class sink;
template<typename T> class source;

//channel primitive, generally this should not be used directly
template<typename T>
class primitive
{
	template<typename U> friend class select;
public:
	//0 buf_size means unbounded buffer size
	explicit primitive(const unsigned buf_size = 0):
		W(new wrap(buf_size))
	{}

	//clear channel, return number of messages erased
	unsigned clear()
	{
		boost::mutex::scoped_lock lock(W->mutex);
		unsigned cnt = W->queue.size();
		W->queue.clear();
		return cnt;
	}

	T recv() const
	{
		boost::mutex::scoped_lock lock(W->mutex);
		while(W->queue.empty()){
			W->producer_cond.wait(W->mutex);
		}
		T tmp = W->queue.front();
		W->queue.pop_front();
		W->consumer_cond.notify_one();
		if(W->source_call_back){
			W->source_call_back();
			W->source_call_back.clear();
		}
		return tmp;
	}

	void send(T t) const
	{
		boost::mutex::scoped_lock lock(W->mutex);
		if(W->buf_size != 0){
			while(W->queue.size() >= W->buf_size){
				W->consumer_cond.wait(W->mutex);
			}
		}
		W->queue.push_back(t);
		W->producer_cond.notify_one();
		if(W->sink_call_back){
			W->sink_call_back();
			W->sink_call_back.clear();
		}
	}

	bool operator < (const primitive & rval) const
	{
		return W < rval.W;
	}

private:
	class wrap
	{
	public:
		explicit wrap(const unsigned buf_size_in):
			buf_size(buf_size_in)
		{}
		boost::mutex mutex;
		unsigned buf_size;
		boost::condition_variable_any consumer_cond;
		boost::condition_variable_any producer_cond;
		typename std::list<T> queue;
		boost::function<void ()> source_call_back;
		boost::function<void ()> sink_call_back;
	};
	boost::shared_ptr<wrap> W;

	//call back when ready to recv
	void select_sink(boost::function<void ()> call_back) const
	{
		boost::mutex::scoped_lock lock(W->mutex);
		assert(!W->sink_call_back);
		if(W->queue.empty()){
			W->sink_call_back = call_back;
		}else{
			call_back();
		}
	}

	//call back when ready to send
	void select_source(boost::function<void ()> call_back) const
	{
		boost::mutex::scoped_lock lock(W->mutex);
		assert(!W->source_call_back);
		if(W->queue.size() >= W->buf_size){
			W->source_call_back = call_back;
		}else{
			call_back();
		}
	}
};

//decorator for primitive, only allows recv
template<typename T>
class sink
{
	template<typename U> friend class select;
	template<typename U> friend std::pair<source<U>, sink<U> >
		make_chan(const unsigned buf_size);
public:
	T recv() const
	{
		return chan.recv();
	}

	bool operator < (const sink & rval) const
	{
		return chan < rval.chan;
	}

private:
	explicit sink(primitive<T> chan_in):
		chan(chan_in)
	{}

	primitive<T> chan;
};

//decorator for primitive, only allows send
template<typename T>
class source
{
	template<typename U> friend class select;
	template<typename U> friend std::pair<source<U>, sink<U> >
		make_chan(const unsigned buf_size);
public:
	void send(T t) const
	{
		chan.send(t);
	}

	bool operator < (const source & rval) const
	{
		return chan < rval.chan;
	}

private:
	explicit source(primitive<T> chan_in):
		chan(chan_in)
	{}

	primitive<T> chan;
};

/*
A future value that has yet to be computed. Dereferencing this object will block
until the value has been computed.
*/
template<typename T>
class future
{
	template<typename U> friend class select;
	template<typename U> friend std::pair<promise<U>, future<U> > make_future();
public:
	//recv value from promise, or return cached value if already recv'd
	T operator * () const
	{
		boost::mutex::scoped_lock lock(W->mutex);
		if(W->val){
			return *W->val;
		}else{
			W->val = W->Sink.recv();
			return *W->val;
		}
	}

private:
	explicit future(sink<T> & Sink):
		W(new wrap(Sink))
	{}

	class wrap
	{
	public:
		explicit wrap(sink<T> & Sink_in):
			Sink(Sink_in)
		{}
		boost::mutex mutex;
		sink<T> Sink;
		boost::optional<T> val;
	};
	boost::shared_ptr<wrap> W;
};

//a promise to compute a value, very similar to a source
template<typename T>
class promise
{
	template<typename U> friend class select;
	template<typename U> friend std::pair<promise<U>, future<U> > make_future();
public:
	void operator = (const T & rval)
	{
		boost::mutex::scoped_lock lock(W->mutex);
		assert(!W->fulfilled);
		W->fulfilled = true;
		W->Source.send(rval);
	}

private:
	explicit promise(source<T> & Source):
		W(new wrap(Source))
	{}

	class wrap
	{
	public:
		explicit wrap(source<T> & Source_in):
			Source(Source_in),
			fulfilled(false)
		{}
		boost::mutex mutex;
		source<T> Source;
		bool fulfilled;
	};
	boost::shared_ptr<wrap> W;
};

/*
Demultiplex messages from multiple channels. Acts like the BSD sockets select()
function. A set of channels are passed to select, when the function returns the
set contains only channels ready for send (in the case of sources) or recv (in
the case of sinks).
*/
template<typename T>
class select
{
public:
	//select on source channels
	void operator () (std::set<source<T> > & source_set_in)
	{
		std::set<sink<T> > tmp;
		(*this)(source_set_in, tmp);
	}

	//select on sink channels
	void operator () (std::set<sink<T> > & sink_set_in)
	{
		std::set<source<T> > tmp;
		(*this)(tmp, sink_set_in);
	}

	//select on both source and sink channels
	void operator () (std::set<source<T> > & source_set_in,
		std::set<sink<T> > & sink_set_in)
	{
		for(typename std::set<source<T> >::iterator it_cur = source_set_in.begin(),
			it_end = source_set_in.end(); it_cur != it_end; ++it_cur)
		{
			it_cur->chan.select_source(boost::bind(&select::source_call_back, this, *it_cur));
		}
		for(typename std::set<sink<T> >::iterator it_cur = sink_set_in.begin(),
			it_end = sink_set_in.end(); it_cur != it_end; ++it_cur)
		{
			it_cur->chan.select_sink(boost::bind(&select::sink_call_back, this, *it_cur));
		}
		{//BEGIN lock scope
		boost::mutex::scoped_lock lock(mutex);
		while(source_set.empty() && sink_set.empty()){
			cond.wait(mutex);
		}
		source_set_in = source_set;
		sink_set_in = sink_set;
		source_set.clear();
		sink_set.clear();
		}//END lock scope
	}

private:
	boost::mutex mutex;
	boost::condition_variable_any cond;
	std::set<source<T> > source_set;
	std::set<sink<T> > sink_set;

	void source_call_back(source<T> ch)
	{
		boost::mutex::scoped_lock lock(mutex);
		source_set.insert(ch);
		cond.notify_one();
	}

	void sink_call_back(sink<T> ch)
	{
		boost::mutex::scoped_lock lock(mutex);
		sink_set.insert(ch);
		cond.notify_one();
	}
};

template<typename T>
static std::pair<source<T>, sink<T> > make_chan(const unsigned buf_size = 0)
{
	primitive<T> tmp(buf_size);
	return std::make_pair(source<T>(tmp), sink<T>(tmp));
}

template<typename T>
static std::pair<promise<T>, future<T> > make_future()
{
	std::pair<source<T>, sink<T> > chan_pair = make_chan<T>();
	return std::make_pair(promise<T>(chan_pair.first), future<T>(chan_pair.second));
}

}//end namespace channel
#endif
