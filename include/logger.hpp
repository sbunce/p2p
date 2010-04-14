#ifndef H_LOGGER
#define H_LOGGER

//include
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

//standard
#include <iostream>
#include <sstream>

#define LOGGER logger<>::create(__FILE__, __FUNCTION__, __LINE__, "deprecated")
#define LOG(LEVEL) logger<>::create(__FILE__, __FUNCTION__, __LINE__, (LEVEL))

/*
Template type only used to initialize static data members in header. This is
used for the once_flag.
*/
template<typename DUMMY=int>
class logger
{
public:
	~logger()
	{
		boost::mutex::scoped_lock lock(stdout_mutex());
		std::cout << level << ":" << file << ":" << func << ":" << line << " " << buf.str() << "\n";
	}

	static logger<> create(const std::string & file, const std::string & func,
		const int line, const std::string & level)
	{
		return logger<>(file, func, line, level);
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
		const int line_in,
		const std::string & level_in
	):
		file(file_in),
		func(func_in),
		line(line_in),
		level(level_in)
	{
		boost::call_once(init, once_flag);
	}

	logger(const logger & L):
		file(L.file),
		func(L.func),
		line(L.line),
		level(L.level)
	{
		buf << L.buf.str();
	}

	std::string file;
	std::string func;
	int line;
	std::string level;
	std::stringstream buf;

	//used to instantiate static data members in thread safe way
	static boost::once_flag once_flag;
	static void init()
	{
		stdout_mutex();
	}

	/*
	Mutex to lock access to stdout, stop threads from stepping on eachothers
	output to stdout.
	*/
	static boost::mutex & stdout_mutex()
	{
		static boost::mutex M;
		return M;
	}
};

template<typename DUMMY> boost::once_flag logger<DUMMY>::once_flag = BOOST_ONCE_INIT;
#endif
