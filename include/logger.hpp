#ifndef H_LOGGER
#define H_LOGGER

//include
#include <boost/thread.hpp>

//standard
#include <iostream>
#include <sstream>

#define LOG logger::create(__FILE__, __FUNCTION__, __LINE__)

class logger
{
public:
	~logger()
	{
		boost::mutex::scoped_lock lock(static_wrap<>::get().stdout_mutex);
		std::cout << "[" << file << "][" << func << "][" << line << "] " << buf.str() << "\n";
	}

	static logger create(const std::string & file, const std::string & func,
		const int line)
	{
		return logger(file, func, line);
	}

	template<typename T>
	logger & operator << (const T & t)
	{
		buf << t;
		return *this;
	}

private:
	logger(
		const std::string & file_in,
		const std::string & func_in,
		const int line_in
	):
		file(file_in),
		func(func_in),
		line(line_in)
	{}

	logger(const logger & L):
		file(L.file),
		func(L.func),
		line(L.line)
	{
		buf << L.buf.str();
	}

	std::string file;
	std::string func;
	int line;
	std::stringstream buf;

	template<typename T=int>
	class static_wrap
	{
	public:
		class static_objects
		{
		public:
			//locks all access to stdout
			boost::mutex stdout_mutex;
		};

		//get access to static objects
		static static_objects & get()
		{
			boost::call_once(once_flag, _get);
			return _get();
		}
	private:
		static boost::once_flag once_flag;
		static static_objects & _get()
		{
			static static_objects SO;
			return SO;
		}
	};
};

template<typename T> boost::once_flag logger::static_wrap<T>::once_flag = BOOST_ONCE_INIT;
#endif
