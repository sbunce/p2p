/*
This is a reference counting locking pointer AKA "locking smart pointer".

Accessing the object within this class using the -> opterator will be locked:
ex: locking_smart_pointer<my_class> x(new my_class);
    x->func();

When using the * dereference operator an extra * will be needed. The returned
proxy object must be "dereferenced":
ex: locking_smart_ptr<int> x(new int);
    **x = 0;

Problems and why they can't happen:

Events:
1. dtor entered by thread 1
2. copy ctor entered by thread 2 and mutex hit
3. thread 1 deletes everything because it's the laste reference
4. copy ctor deadlocks

This scenario can't happen because in order to delete resources for the locking
pointer there can only be one reference. If there is only one reference there is
no way another thread could call the copy ctor. The only thread that can get to
the copy ctor is already in the dtor.
*/

#ifndef H_LOCKING_SMART_POINTER
#define H_LOCKING_SMART_POINTER

//boost
#include <boost/thread/mutex.hpp>

template <class T>
class locking_smart_pointer
{
public:
	//ctor
	explicit locking_smart_pointer(T * pointee_in)
	{
		pointee = pointee_in;
		pointee_mutex = new boost::mutex;
		ref_count = new int(1);
		ref_count_mutex = new boost::mutex;
	}
 
	//copy ctor
	locking_smart_pointer(const locking_smart_pointer & lp)
	{
		boost::mutex::scoped_lock l1(*lp.pointee_mutex);
		boost::mutex::scoped_lock l2(*lp.ref_count_mutex);
		pointee = lp.pointee;
		pointee_mutex = lp.pointee_mutex;
		ref_count = lp.ref_count;
		ref_count_mutex = lp.ref_count_mutex;
		++*ref_count;
	}

	~locking_smart_pointer()
	{
		ref_count_mutex->lock();
		if(*ref_count == 1){
			//last reference, clean up
			ref_count_mutex->unlock();
			delete pointee;
			delete pointee_mutex;
			delete ref_count;
			delete ref_count_mutex;
		}else{
			--*ref_count;
			ref_count_mutex->unlock();
		}
	}

	class proxy
	{
	public:
		proxy(T * pointee_in, boost::mutex * pointee_mutex_in)
		{
			pointee_mutex = pointee_mutex_in;
			pointee_mutex->lock();
			pointee = pointee_in;
		}

		~proxy()
		{
			pointee_mutex->unlock();
		}

		T * operator -> (){ return pointee; }
		T & operator * (){ return *pointee; }

	private:
		T * pointee;
		boost::mutex * pointee_mutex;
	};

	proxy operator -> (){ return proxy(pointee, pointee_mutex); }
	proxy operator * (){ return proxy(pointee, pointee_mutex); }

private:
	T * pointee;
	boost::mutex * pointee_mutex;
	int * ref_count;
	boost::mutex * ref_count_mutex;
};
#endif
