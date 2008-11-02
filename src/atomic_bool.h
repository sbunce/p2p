/*
This is a wrapper to make a bool atomic.
*/

#ifndef H_ATOMIC_BOOL
#define H_ATOMIC_BOOL

//boost
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

//std
#include <iostream>

class atomic_bool
{
public:
	atomic_bool(){ Mutex = new boost::mutex; }
	atomic_bool(const bool & t): x(t){ Mutex = new boost::mutex; }
	atomic_bool(const atomic_bool & val): x(val.get_value()){ Mutex = new boost::mutex; }
	~atomic_bool(){ delete Mutex; }

	/*
	Operators to interact with atomic_int instantiations.
	*/
	const bool operator = (const atomic_bool & rval)
	{
		boost::mutex::scoped_lock lock(*Mutex);
		return x = rval.get_value();
	}
	const bool operator == (const atomic_bool & rval)
	{
		boost::mutex::scoped_lock lock(*Mutex);
		return x == rval.get_value();
	}
	const bool operator != (const atomic_bool & rval)
	{
		boost::mutex::scoped_lock lock(*Mutex);
		return x != rval.get_value();
	}
	//if(x){}
	operator bool ()
	{
		boost::mutex::scoped_lock lock(*Mutex);
		return x;
	}

	/*
	Operators to interact with bools.
	*/
	const bool operator = (const bool & rval)
	{
		boost::mutex::scoped_lock lock(*Mutex);
		return x = rval;
	}
	const bool operator == (const bool & rval)
	{
		boost::mutex::scoped_lock lock(*Mutex);
		return x == rval;
	}
	const bool operator != (const bool & rval)
	{
		boost::mutex::scoped_lock lock(*Mutex);
		return x != rval;
	}

	/*
	Support using atomic_int in ostream's.
	*/
	friend std::ostream & operator << (std::ostream & lval, const atomic_bool & rval)
	{
		return lval << rval.get_value();
	}

private:
	bool x;
	boost::mutex * Mutex;

	//used to lock access when getting wrapped value
	const bool get_value() const
	{
		boost::mutex::scoped_lock lock(*Mutex);
		return x;
	}
};
#endif
