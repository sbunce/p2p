//THREADSAFE
#ifndef H_ATOMIC_INT
#define H_ATOMIC_INT

//include
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>
#include <logger.hpp>

//standard
#include <iostream>

template<class T>
class atomic_int
{
public:
	atomic_int():
		Mutex(new boost::recursive_mutex)
	{}

	atomic_int(
		const T & wrapped_int_in
	):
		wrapped_int(wrapped_int_in),
		Mutex(new boost::recursive_mutex)
	{}

	atomic_int(
		const atomic_int<T> & val
	):
		wrapped_int(val.get_wrapped_int()),
		Mutex(new boost::recursive_mutex)
	{}

	//assignment (=)
	atomic_int<T> & operator = (const atomic_int<T> & rval)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		wrapped_int = rval.get_wrapped_int();
		return *this;
	}
	atomic_int<T> & operator = (const T & rval)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		wrapped_int = rval;
		return *this;
	}

	//increment/decrement (++, --)
	atomic_int<T> & operator ++ ()
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		++wrapped_int;
		return *this;
	}
	T operator ++ (int)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		T temp = wrapped_int;
		++wrapped_int;
		return temp;
	}
	atomic_int<T> & operator -- (void)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		--wrapped_int;
		return *this;
	}
	T operator -- (int)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		T temp = wrapped_int;
		--wrapped_int;
		return temp;
	}

	//conditional (?, ())
	operator T () const
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		return wrapped_int;
	}

	//compound assignment (+=, -=, *=, /=, %=, >>=, <<=, &=, ^=, |=)
	atomic_int<T> & operator += (const atomic_int<T> & rval)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		wrapped_int += rval.wrapped_int;
		return *this;
	}
	atomic_int<T> & operator += (const T & rval)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		wrapped_int += rval;
		return *this;
	}
	atomic_int<T> & operator -= (const atomic_int<T> & rval)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		wrapped_int -= rval.get_wrapped_int();
		return *this;
	}
	atomic_int<T> & operator -= (const T & rval)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		wrapped_int -= rval;
		return *this;
	}
	atomic_int<T> & operator *= (const atomic_int<T> & rval)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		wrapped_int *= rval.get_wrapped_int();
		return *this;
	}
	atomic_int<T> & operator *= (const T & rval)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		wrapped_int *= rval;
		return *this;
	}
	atomic_int<T> & operator /= (const atomic_int<T> & rval)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		wrapped_int /= rval.get_wrapped_int();
		return *this;
	}
	atomic_int<T> & operator /= (const T & rval)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		wrapped_int /= rval;
		return *this;
	}
	atomic_int<T> & operator %= (const atomic_int<T> & rval)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		wrapped_int %= rval.get_wrapped_int();
		return *this;
	}
	atomic_int<T> & operator %= (const T & rval)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		wrapped_int %= rval;
		return *this;
	}
	atomic_int<T> & operator >>= (const int & rval)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		wrapped_int >>= rval;
		return *this;
	}
	atomic_int<T> & operator <<= (const int & rval)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		wrapped_int <<= rval;
		return *this;
	}
	atomic_int<T> & operator &= (const atomic_int<T> & rval)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		wrapped_int &= rval.get_wrapped_int();
		return *this;
	}
	atomic_int<T> & operator &= (const T & rval)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		wrapped_int &= rval;
		return *this;
	}
	atomic_int<T> & operator ^= (const atomic_int<T> & rval)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		wrapped_int ^= rval.get_wrapped_int();
		return *this;
	}
	atomic_int<T> & operator ^= (const T & rval)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		wrapped_int ^= rval;
		return *this;
	}
	atomic_int<T> & operator |= (const atomic_int<T> & rval)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		wrapped_int |= rval.get_wrapped_int();
		return *this;
	}
	atomic_int<T> & operator |= (const T & rval)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		wrapped_int |= rval;
		return *this;
	}

	//ostream and istream
	friend std::ostream & operator << (std::ostream & lval, const atomic_int<T> & rval)
	{
		return lval << rval.get_wrapped_int();
	}
	friend std::istream & operator >> (std::istream & lval, atomic_int<T> & rval)
	{
		boost::recursive_mutex::scoped_lock lock(*rval.Mutex);
		return lval >> rval.wrapped_int;
	}

private:
	T wrapped_int;

	/*
	A recursive mutex is used because there are instances where the rval is the
	lval. In these instances there would be a deadlock with a regular mutex.

	example: x = x * x; x *= x;
	*/
	boost::scoped_ptr<boost::recursive_mutex> Mutex;

	//used to lock access when getting wrapped value
	const T get_wrapped_int() const
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		return wrapped_int;
	}
};
#endif
