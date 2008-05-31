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
		//double-checked lock eliminates mutex overhead
		if(logger_instance == NULL){ //non-threadsafe comparison (the hint)
			boost::mutex::scoped_lock lock(Mutex);
			if(logger_instance == NULL){ //threadsafe comparison
				logger_instance = new volatile logger();
			}
		}
		std::ostringstream message;
		message << file << ":" << function << ":" << line << " " << T_1;
		((logger *)logger_instance)->write_log(message.str());
	}
	template<class t_1, class t_2>
	static void debug(const char * file, const char * function, int line, t_1 T_1, t_2 T_2)
	{
		if(logger_instance == NULL){
			boost::mutex::scoped_lock lock(Mutex);
			if(logger_instance == NULL){
				logger_instance = new volatile logger();
			}
		}
		std::ostringstream message;
		message << file << ":" << function << ":" << line << " " << T_1 << T_2;
		((logger *)logger_instance)->write_log(message.str());
	}
	template<class t_1, class t_2, class t_3>
	static void debug(const char * file, const char * function, int line, t_1 T_1, t_2 T_2, t_3 T_3)
	{
		if(logger_instance == NULL){
			boost::mutex::scoped_lock lock(Mutex);
			if(logger_instance == NULL){
				logger_instance = new volatile logger();
			}
		}
		std::ostringstream message;
		message << file << ":" << function << ":" << line << " " << T_1 << T_2 << T_3;
		((logger *)logger_instance)->write_log(message.str());
	}
	template<class t_1, class t_2, class t_3, class t_4>
	static void debug(const char * file, const char * function, int line, t_1 T_1, t_2 T_2, t_3 T_3, t_4 T_4)
	{
		if(logger_instance == NULL){
			boost::mutex::scoped_lock lock(Mutex);
			if(logger_instance == NULL){
				logger_instance = new volatile logger();
			}
		}
		std::ostringstream message;
		message << file << ":" << function << ":" << line << " " << T_1 << T_2 << T_3 << T_4;
		((logger *)logger_instance)->write_log(message.str());
	}
	template<class t_1, class t_2, class t_3, class t_4, class t_5>
	static void debug(const char * file, const char * function, int line, t_1 T_1, t_2 T_2, t_3 T_3, t_4 T_4, t_5 T_5)
	{
		if(logger_instance == NULL){
			boost::mutex::scoped_lock lock(Mutex);
			if(logger_instance == NULL){
				logger_instance = new volatile logger();
			}
		}
		std::ostringstream message;
		message << file << ":" << function << ":" << line << " " << T_1 << T_2 << T_3 << T_4 << T_5;
		((logger *)logger_instance)->write_log(message.str());
	}

private:
	//only the logger can initialize itself
	logger();

	//the one possible instance
	static volatile logger * logger_instance;

	//mutex for checking if singleton was initialized
	static boost::mutex Mutex;

	/*
	write_log - does something with the log message
	*/
	void write_log(const std::string & message);
};
#endif
