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
		wrapped_int(val.get()),
		Mutex(new boost::recursive_mutex)
	{}

	//assignment (=)
	atomic_int<T> & operator = (const atomic_int<T> & rval)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		wrapped_int = rval.get();
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
	operator T ()
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		return wrapped_int;
	}

	//compound assignment (+=, -=, *=, /=, %=, >>=, <<=, &=, ^=, |=)
	atomic_int<T> & operator += (const atomic_int<T> & rval)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		wrapped_int += rval.get();
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
		wrapped_int -= rval.get();
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
		wrapped_int *= rval.get();
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
		wrapped_int /= rval.get();
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
		wrapped_int %= rval.get();
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
		wrapped_int &= rval.get();
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
		wrapped_int ^= rval.get();
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
		wrapped_int |= rval.get();
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
		return lval << rval.get();
	}
	friend std::istream & operator >> (std::istream & lval, atomic_int<T> & rval)
	{
		boost::recursive_mutex::scoped_lock lock(*rval.Mutex);
		return lval >> rval.wrapped_int;
	}

private:
	T wrapped_int;

	/*
	The recursive_mutex is because rval may equal lval. The scoped_ptr needed
	to have const member functions with lock objects.
	*/
	boost::scoped_ptr<boost::recursive_mutex> Mutex;

	//used to lock access when getting wrapped value
	const T get() const
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		return wrapped_int;
	}
};
#endif
