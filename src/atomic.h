#ifndef H_ATOMIC
#define H_ATOMIC

/*
This container locks access to a variable. Operators have been overloaded such
that the container can be treated like the variable you put in to it. Not all
operators are present for all types.
*/

//std
#include <iostream>

//boost
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

//predecl
template<class T> class atomic;
template<class T> std::ostream & operator <<(std::ostream &, atomic<T> &);

template<class T> class atomic
{
public:
	atomic();
	atomic(T & t);

	//operators to interact with the type atomic class holds
	const T operator = (const T & rval);
	bool operator == (const T & rval);
	bool operator != (const T & rval);
	const T operator ++ (void); //pre-increment
	const T operator ++ (int);  //post-increment
	const T operator -- (void); //pre-decrement
	const T operator -- (int);  //post-decrement
	operator bool ();           //if(atomicObj){}

	//operators to interact with other atomic instantiations
	const atomic<T> & operator = (atomic<T> & rval);
	bool operator == (atomic<T> & rval);
	bool operator != (atomic<T> & rval);
	friend std::ostream & operator << <T>(std::ostream & lval, atomic<T> & rval);

private:
	T x;
	boost::mutex Mutex;

	//used by friend functions to retrieve x (needed to lock access to x)
	const T get_value();
};

template<class T> atomic<T>::atomic()
{
}
template<class T> atomic<T>::atomic(T & t)
{
	x = t;
}
template<class T> const T atomic<T>::get_value()
{
	{ boost::mutex::scoped_lock lock(Mutex);
	return x;
	}
}

//operators to interact with the type atomic class holds
template<class T> const T atomic<T>::operator = (const T & rval)
{
//	{ boost::mutex::scoped_lock lock(Mutex);
	return x = rval;
//	}
}
template<class T> bool atomic<T>::operator == (const T & rval)
{
//	{ boost::mutex::scoped_lock lock(Mutex);
	return x == rval;
//	}
}
template<class T> bool atomic<T>::operator != (const T & rval)
{
//	{ boost::mutex::scoped_lock lock(Mutex);
	return x != rval;
//	}
}
template<class T> const T atomic<T>::operator ++ (void)
{
//	{ boost::mutex::scoped_lock lock(Mutex);
	return ++x;
//	}
}
template<class T> const T atomic<T>::operator ++ (int)
{
//	{ boost::mutex::scoped_lock lock(Mutex);
	return x++;
//	}
}
template<class T> const T atomic<T>::operator -- (void)
{
//	{ boost::mutex::scoped_lock lock(Mutex);
	return --x;
//	}
}
template<class T> const T atomic<T>::operator -- (int)
{
//	{ boost::mutex::scoped_lock lock(Mutex);
	return x--;
//	}
}
template<class T> atomic<T>::operator bool ()
{
//	{ boost::mutex::scoped_lock lock(Mutex);
	if(x){ return true;}
	else{ return false;}
//	}
}

//operators to interact with other atomic classes
template<class T> const atomic<T> & atomic<T>::operator = (atomic<T> & rval)
{
//	{ boost::mutex::scoped_lock lock(Mutex);
	x = rval.get_value();
	return *this;
//	}
}
template<class T> bool atomic<T>::operator == (atomic<T> & rval)
{
//	{ boost::mutex::scoped_lock lock(Mutex);
	return x == rval.get_value();
//	}
}
template<class T> bool atomic<T>::operator != (atomic<T> & rval)
{
//	{ boost::mutex::scoped_lock lock(Mutex);
	return x != rval.get_value();
//	}
}
template<class T> std::ostream & operator << (std::ostream & lval, atomic<T> & rval)
{
	return lval << (T)rval.get_value();
}
#endif

