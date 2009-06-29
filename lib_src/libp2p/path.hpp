#ifndef H_PATH
#define H_PATH

//include
#include <boost/thread.hpp>
#include <logger.hpp>

//standard
#include <cstdlib>
#include <sstream>
#include <string>

class path
{
public:
	/*
	database:
		Path to database.
	main:
		Main program directory.
	download:
		Directory for finished downloads.
	download_unfinished:
		Directory for unfinished downloads.
	rightside_up:
		Location of scratch file for rightside_up hash tree. The thread id is
		appended so different threads will get directed to different files.
	share:
		Directory for shared files.
	temp:
		Returns location of temporary directory.
	upside_down:
		Location of scratch file for upside_down hash tree. The thread id is
		appended so different threads will get directed to different files.
	*/
	static std::string database();
	static std::string main();
	static std::string download();
	static std::string download_unfinished();
	static std::string rightside_up();
	static std::string share();
	static std::string temp();
	static std::string upside_down();

private:
	path();

	/*
	Static variables and means to initialize them. Do not use these variables
	direclty. Use the accessor functions.
	*/
	static boost::once_flag once_flag;
	static void init();
	static std::string * _home;

	/* Accessor functions for static variables.
	home:
		Path to home directory. Or empty string if not defined.
	*/
	static std::string & home();
};
#endif
