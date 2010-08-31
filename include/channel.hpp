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

//predeclarations for friend'ing
template<typename T> class source;
template<typename T> class promise;

/*
Channel primitive. Generally this should not be used directly because everyone
who has a copy can send and receive. However, this might be desired in some
situations.
*/
template<typename T>
class primitive
{
public:
	//0 buf_size means unbounded buffer size
	explicit primitive(const unsigned buf_size = 0):
		Wrap(new wrap(buf_size))
	{}

	//call back when ready to recv
	bool call_back_recv(const boost::function<void ()> & func) const
	{
		boost::mutex::scoped_lock lock(Wrap->mutex);
		assert(!Wrap->recv_call_back);
		if(Wrap->queue.empty()){
			Wrap->recv_call_back = func;
			return false;
		}else{
			return true;
		}
	}

	//call back when ready to send
	bool call_back_send(const boost::function<void ()> & func) const
	{
		boost::mutex::scoped_lock lock(Wrap->mutex);
		assert(!Wrap->send_call_back);
		if(Wrap->queue.size() >= Wrap->buf_size){
			Wrap->send_call_back = func;
			return false;
		}else{
			return true;
		}
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
	friend class source<T>;
public:
	//see documentation for channel::primitive::call_back_recv
	bool call_back(const boost::function<void ()> & func) const
	{
		return chan.call_back_recv(func);
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
	explicit sink(primitive<T> chan_in):
		chan(chan_in)
	{}

	primitive<T> chan;
};

//decorator for primitive, only allows send
template<typename T>
class source
{
public:
	explicit source(const unsigned buf_size = 0):
		chan(primitive<T>(buf_size))
	{}

	//see documentation for channel::primitive::call_back_send
	bool call_back(const boost::function<void ()> & func) const
	{
		return chan.call_back_send(func);
	}

	sink<T> get_sink() const
	{
		return sink<T>(chan);
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
	friend class promise<T>;
public:
	//see documentation for channel::primitive::call_back_recv
	bool call_back(const boost::function<void ()> & func) const
	{
		return Wrap->Sink.call_back_recv(func);
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
	explicit future(sink<T> Sink):
		Wrap(new wrap(Sink))
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
	boost::shared_ptr<wrap> Wrap;
};

//a promise to compute a value, very similar to a source
template<typename T>
class promise
{
public:
	promise():
		Wrap(new wrap())
	{}

	//see documentation for channel::primitive::call_back_send
	bool call_back(const boost::function<void ()> & func) const
	{
		return Wrap->Source.call_back_send(func);
	}

	future<T> get_future() const
	{
		return future<T>(Wrap->Source.get_sink());
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
		explicit wrap():
			Source(source<T>(0)),
			fulfilled(false)
		{}
		boost::mutex mutex;
		source<T> Source;
		bool fulfilled;
	};
	boost::shared_ptr<wrap> Wrap;
};

class select_proxy
{
public:
	select_proxy():
		Wrap(new wrap())
	{}

	template<typename T>
	select_proxy operator () (const std::set<T> & complete, std::set<T> & ready)
	{
		boost::shared_ptr<chan_set_wrap<T> > CSW(new chan_set_wrap<T>(complete, ready));
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
		chan_set_wrap(
			const std::set<T> & complete_in,
			std::set<T> & ready_in
		):
			complete(complete_in),
			ready(ready_in),
			finalized(false)
		{}

		//start monitoring channels for readyness
		void monitor(const boost::shared_ptr<chan_set_wrap<T> > & CSW,
			boost::shared_ptr<ready_cond> RC)
		{
			boost::mutex::scoped_lock lock(mutex);
			for(typename std::set<T>::const_iterator it_cur = complete.begin(),
				it_end = complete.end(); it_cur != it_end; ++it_cur)
			{
				if(it_cur->call_back(boost::bind(&chan_set_wrap::ready_call_back,
					this, CSW, RC, *it_cur)))
				{
					//channel ready immediately
					tmp_ready.insert(*it_cur);
					RC->notify();
				}
			}
		}

		//stop monitoring channels
		virtual void finalize()
		{
			boost::mutex::scoped_lock lock(mutex);
			assert(!finalized);
			finalized = true;
			ready = tmp_ready;
		}

	private:
		const std::set<T> & complete;
		std::set<T> & ready;

		boost::mutex mutex;    //locks access to everything in this section
		std::set<T> tmp_ready; //call back adds ready chans to this
		bool finalized;        //chans may only be added when this is false

		/*
		Channels call back to this function when ready. The shared_ptr makes it so
		chan_set_wrap object not deleted before channel calls back.
		*/
		void ready_call_back(boost::shared_ptr<chan_set_wrap<T> > CSW,
			boost::shared_ptr<ready_cond> RC, T chan)
		{
			boost::mutex::scoped_lock lock(mutex);
			if(!finalized){
				tmp_ready.insert(chan);
				RC->notify();
			}
		}
	};

	//when this is destroyed we wait for ready channels
	class wrap : private boost::noncopyable
	{
	public:
		wrap():
			RC(new ready_cond())
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
Select on channels in complete set. Ready channels are put in ready_set. This is
functionally similar to the BSD sockets select() function.
*/
template<typename T>
static select_proxy select(const std::set<T> & complete, std::set<T> & ready)
{
	select_proxy SP;
	return SP(complete, ready);
}

}//end namespace channel
#endif
