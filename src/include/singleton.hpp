#ifndef H_SINGLETON
#define H_SINGLETON

//boost
#include <boost/scoped_ptr.hpp>
#include <boost/thread/once.hpp>
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
		t = new T();
	}

	static T * t;
	static boost::once_flag once_flag;
};

template<typename T> T * singleton_base<T>::t(NULL);
template<typename T> boost::once_flag singleton_base<T>::once_flag = BOOST_ONCE_INIT;
#endif
