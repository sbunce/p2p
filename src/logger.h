#ifndef H_LOGGER
#define H_LOGGER

//this is more conventient to use for the first 3 parameters function
#define LOGGER_P1 __FILE__, __FUNCTION__, __LINE__

//boost
#include <boost/thread/mutex.hpp>

//std
#include <iostream>
#include <sstream>
#include <string>

//custom
#include "global.h"

class logger
{
public:
	//replace this mess with a variadic template when C++0x comes out
	template<class t_1>
	static void debug(const char * file, const char * function, int line, t_1 T_1)
	{
		init();
		std::ostringstream message;
		message << file << ":" << function << ":" << line << " " << T_1;
		((logger *)Logger)->write_log(message.str());
	}
	template<class t_1, class t_2>
	static void debug(const char * file, const char * function, int line, t_1 T_1, t_2 T_2)
	{
		init();
		std::ostringstream message;
		message << file << ":" << function << ":" << line << " " << T_1 << T_2;
		((logger *)Logger)->write_log(message.str());
	}
	template<class t_1, class t_2, class t_3>
	static void debug(const char * file, const char * function, int line, t_1 T_1, t_2 T_2, t_3 T_3)
	{
		init();
		std::ostringstream message;
		message << file << ":" << function << ":" << line << " " << T_1 << T_2 << T_3;
		((logger *)Logger)->write_log(message.str());
	}
	template<class t_1, class t_2, class t_3, class t_4>
	static void debug(const char * file, const char * function, int line, t_1 T_1, t_2 T_2, t_3 T_3, t_4 T_4)
	{
		init();
		std::ostringstream message;
		message << file << ":" << function << ":" << line << " " << T_1 << T_2 << T_3 << T_4;
		((logger *)Logger)->write_log(message.str());
	}
	template<class t_1, class t_2, class t_3, class t_4, class t_5>
	static void debug(const char * file, const char * function, int line, t_1 T_1, t_2 T_2, t_3 T_3, t_4 T_4, t_5 T_5)
	{
		init();
		std::ostringstream message;
		message << file << ":" << function << ":" << line << " " << T_1 << T_2 << T_3 << T_4 << T_5;
		((logger *)Logger)->write_log(message.str());
	}

private:
	//only the logger can initialize itself
	logger();

	static void init()
	{
		//double checked lock to avoid locking overhead
		if(Logger == NULL){ //unsafe comparison, the hint
			boost::mutex::scoped_lock lock(Mutex);
			if(Logger == NULL){ //threadsafe comparison
				Logger = new logger;
			}
		}
	}

	//the one possible instance
	static logger * Logger;

	//mutex for checking if singleton was initialized
	static boost::mutex Mutex;

	/*
	write_log - does something with the log message
	*/
	void write_log(const std::string & message);
};
#endif
