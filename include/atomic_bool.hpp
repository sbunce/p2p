//THREADSAFE

#ifndef H_ATOMIC_BOOL
#define H_ATOMIC_BOOL

//boost
#include <boost/thread.hpp>

//std
#include <iostream>

class atomic_bool
{
public:
	atomic_bool(){ Mutex = new boost::recursive_mutex; }
	atomic_bool(const bool & wrapped_bool_in): wrapped_bool(wrapped_bool_in){ Mutex = new boost::recursive_mutex; }
	atomic_bool(const atomic_bool & val): wrapped_bool(val.get_wrapped_bool()){ Mutex = new boost::recursive_mutex; }
	~atomic_bool(){ delete Mutex; }

	//assignment operators (=)
	const bool operator = (const atomic_bool & rval)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		return wrapped_bool = rval.get_wrapped_bool();
	}
	const bool operator = (const bool & rval)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		return wrapped_bool = rval;
	}

	//relational (==, !=)
	const bool operator == (const atomic_bool & rval)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		return wrapped_bool == rval.get_wrapped_bool();
	}
	const bool operator == (const bool & rval)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		return wrapped_bool == rval;
	}
	const bool operator != (const atomic_bool & rval)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		return wrapped_bool != rval.get_wrapped_bool();
	}
	const bool operator != (const bool & rval)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		return wrapped_bool != rval;
	}

	//conditional (?, ())
	operator bool ()
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		return wrapped_bool;
	}

	//ostream and istream 
	friend std::ostream & operator << (std::ostream & lval, const atomic_bool & rval)
	{
		return lval << rval.get_wrapped_bool();
	}
	friend std::istream & operator >> (std::istream & lval, atomic_bool & rval)
	{
		boost::recursive_mutex::scoped_lock lock(*rval.Mutex);
		return lval >> rval.wrapped_bool;
	}

private:
	bool wrapped_bool;

	/*
	A recursive mutex is used because there are instances where the rval is the
	lval. In these instances there would be a deadlock with a regular mutex.

	example: x && x; //although who knows why someone would do this..
	*/
	boost::recursive_mutex * Mutex;

	//used to lock access when getting wrapped value
	const bool get_wrapped_bool() const
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		return wrapped_bool;
	}
};
#endif
