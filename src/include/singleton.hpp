#ifndef H_SINGLETON
#define H_SINGLETON

//boost
#include <boost/scoped_ptr.hpp>
#include <boost/thread/once.hpp>
#include <boost/utility.hpp>

/*
Note: The singleton is never deleted because we can't guarantee destruction
order.
*/
template<class T>
class singleton : private boost::noncopyable
{
public:
	static T & instance()
	{
		boost::call_once(init, flag);
		return *t;
	}
protected:
	singleton(){}
	virtual ~singleton(){}

private:
	static void init()
	{
		t = new T();
	}

	static T * t;
	static boost::once_flag flag;
};

template<class T> T * singleton<T>::t(NULL);
template<class T> boost::once_flag singleton<T>::flag = BOOST_ONCE_INIT;
#endif
