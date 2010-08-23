#ifndef H_CHANNEL
#define H_CHANNEL

//include
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <logger.hpp>

//std
#include <list>
#include <set>

namespace channel{

//channel primitive, generally this should not be used directly
template<typename T>
class primitive
{
public:
	//0 buf_size means unbounded buffer size
	explicit primitive(const unsigned buf_size = 0):
		Wrap(new wrap(buf_size))
	{}

	//call back when ready to recv
	void call_back_recv(const boost::function<void ()> & func) const
	{
		boost::mutex::scoped_lock lock(Wrap->mutex);
		assert(!Wrap->recv_call_back);
		if(Wrap->queue.empty()){
			Wrap->recv_call_back = func;
		}else{
			func();
		}
	}

	//call back when ready to send
	void call_back_send(const boost::function<void ()> & func) const
	{
		boost::mutex::scoped_lock lock(Wrap->mutex);
		assert(!Wrap->send_call_back);
		if(Wrap->queue.size() >= Wrap->buf_size){
			Wrap->send_call_back = func;
		}else{
			func();
		}
	}

	//clear channel, return number of messages erased
	unsigned clear() const
	{
		boost::mutex::scoped_lock lock(Wrap->mutex);
		unsigned cnt = Wrap->queue.size();
		Wrap->queue.clear();
		return cnt;
	}

	T recv() const
	{
		boost::mutex::scoped_lock lock(Wrap->mutex);
		while(Wrap->queue.empty()){
			Wrap->producer_cond.wait(Wrap->mutex);
		}
		T tmp = Wrap->queue.front();
		Wrap->queue.pop_front();
		Wrap->consumer_cond.notify_one();
		if(Wrap->send_call_back){
			Wrap->send_call_back();
			Wrap->send_call_back.clear();
		}
		return tmp;
	}

	void send(T t) const
	{
		boost::mutex::scoped_lock lock(Wrap->mutex);
		if(Wrap->buf_size != 0){
			while(Wrap->queue.size() >= Wrap->buf_size){
				Wrap->consumer_cond.wait(Wrap->mutex);
			}
		}
		Wrap->queue.push_back(t);
		Wrap->producer_cond.notify_one();
		if(Wrap->recv_call_back){
			Wrap->recv_call_back();
			Wrap->recv_call_back.clear();
		}
	}

	bool operator < (const primitive & rval) const
	{
		return Wrap < rval.Wrap;
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
		boost::function<void ()> recv_call_back;
		boost::function<void ()> send_call_back;
	};
	boost::shared_ptr<wrap> Wrap;
};

//decorator for primitive, only allows recv
template<typename T>
class sink
{
public:
	explicit sink(primitive<T> chan_in):
		chan(chan_in)
	{}

	void call_back(const boost::function<void ()> & func) const
	{
		chan.call_back_recv(func);
	}

	T recv() const
	{
		return chan.recv();
	}

	bool operator < (const sink & rval) const
	{
		return chan < rval.chan;
	}

private:
	primitive<T> chan;
};

//decorator for primitive, only allows send
template<typename T>
class source
{
public:
	explicit source(primitive<T> chan_in):
		chan(chan_in)
	{}

	void call_back(const boost::function<void ()> & func) const
	{
		chan.call_back_send(func);
	}

	void send(T t) const
	{
		chan.send(t);
	}

	bool operator < (const source & rval) const
	{
		return chan < rval.chan;
	}

private:
	primitive<T> chan;
};

/*
A future value that has yet to be computed. Dereferencing this object will block
until the value has been computed.
*/
template<typename T>
class future
{
public:
	explicit future(sink<T> & Sink):
		Wrap(new wrap(Sink))
	{}

	void call_back(const boost::function<void ()> & func) const
	{
		Wrap->Sink.call_back_recv(func);
	}

	//recv value from promise, or return cached value if already recv'd
	T operator * () const
	{
		boost::mutex::scoped_lock lock(Wrap->mutex);
		if(Wrap->val){
			return *Wrap->val;
		}else{
			Wrap->val = Wrap->Sink.recv();
			return *Wrap->val;
		}
	}

	bool operator < (const future & rval) const
	{
		return Wrap->Sink < rval.Wrap->Sink;
	}

private:
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
	boost::shared_ptr<wrap> Wrap;
};

//a promise to compute a value, very similar to a source
template<typename T>
class promise
{
public:
	explicit promise(source<T> & Source):
		Wrap(new wrap(Source))
	{}

	void call_back(const boost::function<void ()> & func) const
	{
		Wrap->Source.call_back_send(func);
	}

	void operator = (const T & rval) const
	{
		boost::mutex::scoped_lock lock(Wrap->mutex);
		assert(!Wrap->fulfilled);
		Wrap->fulfilled = true;
		Wrap->Source.send(rval);
	}

private:
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
	boost::shared_ptr<wrap> Wrap;
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

class select_proxy
{
public:
	select_proxy():
		Wrap(new wrap())
	{}

	template<typename T>
	select_proxy operator () (std::set<T> & chan_set)
	{
		boost::shared_ptr<chan_set_wrap<T> > CSW(new chan_set_wrap<T>(chan_set));
		CSW->monitor(CSW, Wrap->RC);
		Wrap->Sets.push_back(CSW);
		return *this;
	}

private:
	//blocks until at least one channel ready
	class ready_cond
	{
	public:
		ready_cond():
			ready(false)
		{}

		void wait()
		{
			boost::mutex::scoped_lock lock(mutex);
			while(!ready){
				cond.wait(mutex);
			}
		}

		void notify()
		{
			boost::mutex::scoped_lock lock(mutex);
			ready = true;
			cond.notify_all();
		}

	private:
		boost::mutex mutex;
		boost::condition_variable_any cond;
		bool ready;
	};

	class chan_set_wrap_base : private boost::noncopyable
	{
	public:
		virtual void finalize() = 0;
	};

	template<typename T>
	class chan_set_wrap : public chan_set_wrap_base
	{
	public:
		chan_set_wrap(std::set<T> & chan_set_in):
			chan_set(chan_set_in),
			finalized(false)
		{}

		//start monitoring channels for readyness
		void monitor(const boost::shared_ptr<chan_set_wrap<T> > & CSW,
			boost::shared_ptr<ready_cond> RC)
		{
			boost::recursive_mutex::scoped_lock lock(mutex);
			for(typename std::set<T>::iterator it_cur = chan_set.begin(),
				it_end = chan_set.end(); it_cur != it_end; ++it_cur)
			{
				it_cur->call_back(boost::bind(&chan_set_wrap::ready_call_back, this,
					CSW, RC, *it_cur));
			}
		}

		/*
		Copy ready_chan_set to chan_set. Make it so chan_set can no longer be
		modified.
		*/
		virtual void finalize()
		{
			boost::recursive_mutex::scoped_lock lock(mutex);
			assert(!finalized);
			finalized = true;
			chan_set = ready_chan_set;
		}

	private:
		boost::recursive_mutex mutex; //locks access to chan_set_ready
		std::set<T> & chan_set;       //channel set to select on
		std::set<T> ready_chan_set;
		bool finalized;

		/*
		Channels call back to this function when ready. The shared_ptr makes it so
		chan_set_wrap object not deleted before channel calls back.
		*/
		void ready_call_back(boost::shared_ptr<chan_set_wrap<T> > CSW,
			boost::shared_ptr<ready_cond> RC, T ready_chan)
		{
			boost::recursive_mutex::scoped_lock lock(mutex);
			if(!finalized){
				ready_chan_set.insert(ready_chan);
				RC->notify();
			}
		}
	};

	//when this is destroyed we wait for ready channels
	class wrap : private boost::noncopyable
	{
	public:
		wrap():
			RC(new ready_cond)
		{}

		~wrap()
		{
			RC->wait();
			for(std::list<boost::shared_ptr<chan_set_wrap_base> >::iterator
				it_cur = Sets.begin(), it_end = Sets.end(); it_cur != it_end; ++it_cur)
			{
				(*it_cur)->finalize();
			}
		}

		boost::shared_ptr<ready_cond> RC;
		std::list<boost::shared_ptr<chan_set_wrap_base> > Sets;
	};

	boost::shared_ptr<wrap> Wrap;
};

/*
The select() function acts like the BSD sockets select() function. It takes a
set of channels and when it returns only ready channels are left in the set.
Note: The select_proxy allows chaining.
Example: channel::select(source_set)(sink_set)
*/
template<typename T>
static select_proxy select(std::set<T> & chan_set)
{
	select_proxy SP;
	return SP(chan_set);
}

}//end namespace channel
#endif
