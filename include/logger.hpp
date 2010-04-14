#ifndef H_LOGGER
#define H_LOGGER

//include
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

//standard
#include <iostream>
#include <sstream>

#define LOGGER logger<>::create(__FILE__, __FUNCTION__, __LINE__)
#define LOG(LEVEL) logger<>::create(__FILE__, __FUNCTION__, __LINE__, (LEVEL))

/*
Template type only needed to initialize static members in header.
*/
template<typename DUMMY=int>
class logger
{
public:
	~logger()
	{
		std::cout << file << ":" << func << ":" << line << " " << buf.str() << std::endl;
	}

	static logger<> create(const std::string & file, const std::string & func,
		const int line, const std::string & level = "normal")
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

	static boost::once_flag once_flag;
};

template<typename DUMMY> boost::once_flag logger<DUMMY>::once_flag = BOOST_ONCE_INIT;
#endif
