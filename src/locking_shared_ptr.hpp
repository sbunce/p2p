//THREADSAFE

/*
This is a reference counting locking pointer AKA "locking smart pointer".

Accessing the object within this class using the -> opterator will be locked:
ex: locking_shared_ptr<my_class> x(new my_class);
    x->func();

When using the * dereference operator an extra * will be needed. The returned
proxy object must be "dereferenced":
ex: locking_shared_ptr<int> x(new int);
    **x = 0;

Problems and why they can't happen:

Events:
1. dtor entered by thread 1
2. copy ctor entered by thread 2 and mutex hit
3. thread 1 deletes everything because it's the last reference
4. copy ctor deadlocks

This scenario can't happen because in order to delete resources for the locking
pointer there can only be one reference. If there is only one reference there is
no way another thread could call the copy ctor. The only thread that can get to
the copy ctor is already in the dtor.
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
	/*
	This ctor instantiates an object of typename T. This works if the object has
	no ctor parameters needed. Otherwise the second ctor should be used.
	*/
	locking_shared_ptr()
	:
		pointee(new T()),
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

		T * operator -> (){ return pointee.get(); }
		T & operator * (){ return *pointee; }

	private:
		typename boost::shared_ptr<T> pointee;
		boost::shared_ptr<boost::recursive_mutex> pointee_mutex;
	};

	proxy operator -> (){ return proxy(pointee, pointee_mutex); }
	proxy operator * (){ return proxy(pointee, pointee_mutex); }

	/*
	These are for when multiple expressions must hold the lock on the
	locking_shared_ptr. BE CAREFUL WITH THESE!
	*/
	void lock(){ pointee_mutex->lock(); }
	void unlock(){ pointee_mutex->unlock(); }

private:
	typename boost::shared_ptr<T> pointee;
	boost::shared_ptr<boost::recursive_mutex> pointee_mutex;
};
#endif
