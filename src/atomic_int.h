/*
This is a wrapper meant to be used with integers. It makes access to the wrapped
integer atomic with the use of a mutex.

The template parameter is used to specify what kind of integer you want:
example atomic_int<int>, atomic_int<unsigned int>, etc..
*/

#ifndef H_ATOMIC_INT
#define H_ATOMIC_INT

//boost
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

//std
#include <iostream>

template<class T>
class atomic_int
{
public:
	atomic_int(){ Mutex = new boost::mutex; }
	atomic_int(const T & t): x(t){ Mutex = new boost::mutex; }
	atomic_int(const atomic_int<T> & val): x(val.get_value()){ Mutex = new boost::mutex; }
	~atomic_int(){ delete Mutex; }

	/*
	Operators to interact with atomic_int instantiations.
	*/
	const T operator = (const atomic_int<T> & rval)
	{
		boost::mutex::scoped_lock lock(*Mutex);
		return x = rval.get_value();
	}
	const bool operator == (const atomic_int<T> & rval)
	{
		boost::mutex::scoped_lock lock(*Mutex);
		return x == rval.get_value();
	}
	const bool operator != (const atomic_int<T> & rval)
	{
		boost::mutex::scoped_lock lock(*Mutex);
		return x != rval.get_value();
	}
	const bool operator >= (const atomic_int<T> & rval)
	{
		boost::mutex::scoped_lock lock(*Mutex);
		return x >= rval.get_value();
	}
	const bool operator <= (const atomic_int<T> & rval)
	{
		boost::mutex::scoped_lock lock(*Mutex);
		return x <= rval.get_value();
	}
	//pre-increment
	const T operator ++ ()
	{
		boost::mutex::scoped_lock lock(*Mutex);
		return ++x;
	}
	//post-increment
	const T operator ++ (int)
	{
		boost::mutex::scoped_lock lock(*Mutex);
		return x++;
	}
	//pre-decrement
	const T operator -- (void)
	{
		boost::mutex::scoped_lock lock(*Mutex);
		return --x;
	}
	//post-decrement
	const T operator -- (int)
	{
		boost::mutex::scoped_lock lock(*Mutex);
		return x--;
	}
	//if(x){}
	operator int ()
	{
		boost::mutex::scoped_lock lock(*Mutex);
		return x;
	}

	/*
	Operators to interact with int.
	*/
	const T operator = (const T & rval)
	{
		boost::mutex::scoped_lock lock(*Mutex);
		return x = rval;
	}
	const bool operator == (const T & rval)
	{
		boost::mutex::scoped_lock lock(*Mutex);
		return x == rval;
	}
	const bool operator != (const T & rval)
	{
		boost::mutex::scoped_lock lock(*Mutex);
		return x != rval;
	}
	const bool operator * (const T & rval)
	{
		boost::mutex::scoped_lock lock(*Mutex);
		return x *= rval;
	}

	/*
	Support using atomic_int in ostream's.
	*/
	friend std::ostream & operator << (std::ostream & lval, const atomic_int<T> & rval)
	{
		return lval << rval.get_value();
	}

private:
	T x;
	boost::mutex * Mutex;

	//used to lock access when getting wrapped value
	const T get_value() const
	{
		boost::mutex::scoped_lock lock(*Mutex);
		return x;
	}
};
#endif
