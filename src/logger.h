#ifndef H_LOGGER
#define H_LOGGER

//std
#include <iostream>
#include <sstream>

//custom
#include "global.h"

/*
Example usage of logger:
	LOGGER << "error message " << 123;
*/
#define LOGGER logger::make(__FILE__, __FUNCTION__, __LINE__)

class logger
{
public:
	~logger()
	{
		std::cout << file << ":" << func << ":" << line << " " << buffer.str() << "\n";
	}

	static logger make(const char * file, const char * func, const int & line)
	{
		return logger(file, func, line);
	}

	template <typename T>
	logger & operator << (const T & t)
	{
		buffer << t;
		return *this;
	}

private:
	//only logger::make() can instantiate a logger
	logger(
		const char * file_in,
		const char * func_in,
		const int & line_in
	):
		file(file_in),
		func(func_in),
		line(line_in)
	{}

	/*
	Most likely not used, but possible if temporary not optimized away in
	logger::make()?
	*/
	logger(
		const logger & Logger
	):
		file(Logger.file),
		func(Logger.func),
		line(Logger.line)
	{
		buffer << Logger.buffer.str();
	}

	const char * file, * func;
	int line;
	std::ostringstream buffer;
};
#endif
