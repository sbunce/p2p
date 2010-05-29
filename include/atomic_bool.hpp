//THREADSAFE
#ifndef H_ATOMIC_BOOL
#define H_ATOMIC_BOOL

//include
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>

//standard
#include <iostream>

class atomic_bool
{
public:
	atomic_bool(){}
	atomic_bool(const bool wrapped_bool_in):
		wrapped_bool(wrapped_bool_in)
	{}
	atomic_bool(const atomic_bool & val):
		wrapped_bool(val)
	{}

	//assignment operators (=)
	atomic_bool & operator = (const atomic_bool & rval)
	{
		boost::recursive_mutex::scoped_lock lock(Mutex);
		wrapped_bool = rval;
		return *this;
	}
	atomic_bool & operator = (const bool & rval)
	{
		boost::recursive_mutex::scoped_lock lock(Mutex);
		wrapped_bool = rval;
		return *this;
	}

	//conversion
	operator bool () const
	{
		boost::recursive_mutex::scoped_lock lock(Mutex);
		return wrapped_bool;
	}

	//ostream and istream 
	friend std::ostream & operator << (std::ostream & lval, const atomic_bool & rval)
	{
		boost::recursive_mutex::scoped_lock lock(rval.Mutex);
		return lval << rval.wrapped_bool;
	}
	friend std::istream & operator >> (std::istream & lval, atomic_bool & rval)
	{
		boost::recursive_mutex::scoped_lock lock(rval.Mutex);
		return lval >> rval.wrapped_bool;
	}

private:
	bool wrapped_bool;

	//recursive_mutex is used because rval may equal lval
	mutable boost::recursive_mutex Mutex;

	//used to lock access when getting wrapped value
	bool get() const
	{
		boost::recursive_mutex::scoped_lock lock(Mutex);
		return wrapped_bool;
	}
};
#endif
