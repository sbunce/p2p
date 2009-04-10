//THREADSAFE
/*
Accessing the object within this class using the -> opterator will be locked:
ex: locking_shared_ptr<my_class> x(new my_class);
    x->func();

When using the * dereference operator an extra * will be needed. The returned
proxy object must be "dereferenced":
ex: locking_shared_ptr<int> x(new int);
    **x = 0;
*/

#ifndef H_LOCKING_SHARED_PTR
#define H_LOCKING_SHARED_PTR

//boost
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

template <typename T>
class locking_shared_ptr
{
public:
	//creates empty locking_share_ptr
	locking_shared_ptr()
	:
		pointee_mutex(new boost::recursive_mutex)
	{}

	//this ctor takes a pointer to be wrapped.
	explicit locking_shared_ptr(T * pointee_in)
	:
		pointee(pointee_in),
		pointee_mutex(new boost::recursive_mutex)
	{}

	/*
	Default copy ctor used. The boost::shared_ptr library takes care of
	reference counting and is guaranteed to be thread safe (see boost docs).
	*/

	class proxy
	{
	public:
		proxy(
			typename boost::shared_ptr<T> pointee_in,
			boost::shared_ptr<boost::recursive_mutex> pointee_mutex_in
		):
			pointee(pointee_in),
			pointee_mutex(pointee_mutex_in)
		{
			pointee_mutex->lock();
		}

		~proxy()
		{
			pointee_mutex->unlock();
		}

		T * operator -> () const { return pointee.get(); }
		T & operator * () const { return *pointee; }

	private:
		typename boost::shared_ptr<T> pointee;
		boost::shared_ptr<boost::recursive_mutex> pointee_mutex;
	};

	/*
	DEBUG
	Consider using boost::recursive_mutex ctor param to provide destruction function.
	boost::shared_ptr<boost::recursive_mutex>
		(&pointee_mutex, mem_fn(&boost::recursive_mutex::my_unlock))
	*/
	proxy operator -> () const { return proxy(pointee, pointee_mutex); }
	proxy operator * () const { return proxy(pointee, pointee_mutex); }

	/*
	These are for when multiple expressions must hold the lock on the
	locking_shared_ptr.
	*/
	void lock() const { pointee_mutex->lock(); }
	void unlock() const { pointee_mutex->unlock(); }

	/*
	Returns copy of wrapped pointer. This should generally only be used to
	compare pointers by value. This doesn't do any locking.
	*/
	T * get() const
	{
		return pointee.get();
	}

	//assignment
	locking_shared_ptr<T> & operator = (const locking_shared_ptr & rval)
	{
		if(this == &rval){
			return *this;
		}else{
			/*
			Lock both objects and copy. The mutex will also be copied. This way
			both locking_smart_ptr's protect the pointer to the same object with
			the same mutex.
			*/
			boost::recursive_mutex::scoped_lock(*pointee_mutex);
			boost::recursive_mutex::scoped_lock(*rval.pointee_mutex);
			pointee = rval.pointee;
			return *this;
		}
	}

	//comparison operators
	bool operator == (const locking_shared_ptr<T> & rval)
	{
		boost::recursive_mutex::scoped_lock(*pointee_mutex);
		boost::recursive_mutex::scoped_lock(*rval.pointee_mutex);
		return pointee == rval.pointee;
	}
	bool operator != (const locking_shared_ptr<T> & rval)
	{
		boost::recursive_mutex::scoped_lock(*pointee_mutex);
		boost::recursive_mutex::scoped_lock(*rval.pointee_mutex);
		return pointee != rval.pointee;
	}
	bool operator < (const locking_shared_ptr<T> & rval) const
	{
		boost::recursive_mutex::scoped_lock(*pointee_mutex);
		boost::recursive_mutex::scoped_lock(*rval.pointee_mutex);
		return pointee < rval.pointee;
	}

private:
	typename boost::shared_ptr<T> pointee;
	boost::shared_ptr<boost::recursive_mutex> pointee_mutex;
};
#endif
