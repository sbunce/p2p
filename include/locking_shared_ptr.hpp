/*
Accessing the object within this class using the -> opterator will be locked:
ex: locking_shared_ptr<my_class> x(new my_class);
    x->func();
*/
//THREADSAFE
#ifndef H_LOCKING_SHARED_PTR
#define H_LOCKING_SHARED_PTR

//boost
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

template <typename T>
class locking_shared_ptr
{
public:
	locking_shared_ptr():
		pointee_mutex(new boost::recursive_mutex)
	{}

	explicit locking_shared_ptr(T * pointee_in):
		pointee(pointee_in),
		pointee_mutex(new boost::recursive_mutex)
	{}

	/*
	Default copy ctor used. The boost::shared_ptr library takes care of
	reference counting and is guaranteed to be thread safe.
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

	private:
		typename boost::shared_ptr<T> pointee;
		boost::shared_ptr<boost::recursive_mutex> pointee_mutex;
	};

	proxy operator -> () const { return proxy(pointee, pointee_mutex); }

	//get wrapped pointer, does not do locking
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
			boost::recursive_mutex::scoped_lock(*pointee_mutex);
			boost::recursive_mutex::scoped_lock(*rval.pointee_mutex);
			pointee = rval.pointee;
			pointee_mutex = rval.pointee_mutex;
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
