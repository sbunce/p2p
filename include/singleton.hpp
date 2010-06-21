/* Instructions
Derive a class from singleton_base. The template type should be that of the
derived class. Make the constructor of the derived class private and make the
singleton base class a friend.

example:
class derived : public singleton_base<derived>
{
	friend class singleton_base<number_generator>;
public:

private:
	derived();
};

The singletone can then be accessed via the singleton function.
example:
	derived::singleton().func();

Note: Singleton objects are static so they are destructed in the reverse order
they are constructed. You will have to specify construction order if the
singleton objects ctors or dtors depend on eachother in any way. Specifying the
order can be done by calling the singleton() function in the appropriate order.
*/
#ifndef H_SINGLETON
#define H_SINGLETON

//include
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

template<typename T>
class singleton_base : private boost::noncopyable
{
public:
	/*
	A shared_ptr made available to help solve static initialization problems.
	Example Problem (numbers indicate order events happen):
	0. singleton A instantiated (will be destroyed last).
	1. singleton B instantiated (will be destroyed first).
	2. B destroyed.
	3. A tries to use B (PROGRAM CRASH).

	We can get around this initialization order problem by storing a shared_ptr
	to B within A. This way we can stop B from getting destroyed until A is done
	with it.
	*/
	static const boost::shared_ptr<T> & singleton()
	{
		boost::call_once(&init, once_flag);
		return obj_ptr;
	}

protected:
	singleton_base(){}
	virtual ~singleton_base(){}

private:
	static void init()
	{
		obj_ptr.reset(new T());
	}

	static boost::once_flag once_flag;
	static boost::shared_ptr<T> obj_ptr;
};

template<typename T> boost::shared_ptr<T> singleton_base<T>::obj_ptr;
template<typename T> boost::once_flag singleton_base<T>::once_flag = BOOST_ONCE_INIT;
#endif
