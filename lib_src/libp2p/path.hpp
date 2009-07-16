#ifndef H_PATH
#define H_PATH

//include
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <logger.hpp>

//standard
#include <cstdlib>
#include <sstream>
#include <string>

class path
{
public:
	//creates required directories
	static void create_required_directories();

	/* Files
	database:
		Path to database.
	rightside_up:
		Location of scratch file for rightside_up hash tree. The thread id is
		appended so different threads will get directed to different files.
	upside_down:
		Location of scratch file for upside_down hash tree. The thread id is
		appended so different threads will get directed to different files.
	*/
	static std::string database();
	static std::string rightside_up();
	static std::string upside_down();

	/* Directories
	download:
		Directory for finished downloads.
	main:
		Main program directory.
	share:
		Directory for shared files.
	temp:
		Returns location of temporary directory.
	*/

	static std::string download();
	static std::string main();
	static std::string share();
	static std::string temp();

private:
	path(){}

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
