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
#include <boost/scoped_ptr.hpp>
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
	virtual ~singleton_base(){}

private:
	static void init()
	{
		/*
		The scoped_ptr is guaranteed to never throw. If T ctor throws the
		scoped_ptr will set itself to NULL which will cause an exception when
		singleton() is called. For this reason classes derived from the
		singleton_base must not throw in ctor.
		*/
		t.reset(new T());
	}

	static boost::scoped_ptr<T> t;
	static boost::once_flag once_flag;
};

template<typename T> boost::scoped_ptr<T> singleton_base<T>::t(NULL);
template<typename T> boost::once_flag singleton_base<T>::once_flag = BOOST_ONCE_INIT;
#endif
