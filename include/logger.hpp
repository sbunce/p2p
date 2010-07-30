#ifndef H_LOGGER
#define H_LOGGER

//include
#include <boost/thread.hpp>
#include <singleton.hpp>

//standard
#include <iostream>
#include <sstream>

#define LOG logger::create(__FILE__, __FUNCTION__, __LINE__)

class logger
{
public:
	~logger()
	{
		boost::mutex::scoped_lock lock(wrap::singleton()->stdout_mutex);
		std::cout << "[" << file << "][" << func << "][" << line << "] "
			<< buf.str() << "\n";
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

	class wrap : public singleton_base<wrap>
	{
		friend class singleton_base<wrap>;
	public:
		boost::mutex stdout_mutex;
	};
};
#endif
