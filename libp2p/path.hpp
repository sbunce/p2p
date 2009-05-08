#ifndef H_PATH
#define H_PATH

//boost
#include <boost/thread.hpp>

//include
#include <logger.hpp>

//std
#include <cstdlib>
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
	share:
		Directory for shared files.
	*/
	static std::string database();
	static std::string main();
	static std::string download();
	static std::string download_unfinished();
	static std::string share();

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
