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

Note: The destructor of the derived class will not ever be called. This is done
because of static initialization order issues. If clean up needs to be done a
function should be included in the derived class that can be called at an
appropriate time.
*/
#ifndef H_SINGLETON
#define H_SINGLETON

//include
#include <boost/thread.hpp>
#include <boost/utility.hpp>

template<typename T>
class singleton_base : private boost::noncopyable
{
public:
	static T & singleton()
	{
		boost::call_once(init, once_flag);
		return *t;
	}

protected:
	singleton_base(){}

private:
	static void init()
	{
		t = new T();
	}

	static T * t;
	static boost::once_flag once_flag;
};

template<typename T> T * singleton_base<T>::t(NULL);
template<typename T> boost::once_flag singleton_base<T>::once_flag = BOOST_ONCE_INIT;
#endif
