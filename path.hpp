/*
Functions for anything which involves determining a filesystem path.
*/
#ifndef H_PATH
#define H_PATH

//std
#include <cstdlib>
#include <string>

namespace path
{
//returns path to database
static std::string database()
{
	std::string path;
	path += std::getenv("HOME");
	path += "/.p2p/DB";
	return path;
}

//returns main program directory
static std::string main()
{
	std::string path;
	path += std::getenv("HOME");
	path += "/.p2p/";
	return path;
}

//returns directory for finished downloads
static std::string download()
{
	std::string path;
	path += std::getenv("HOME");
	path += "/.p2p/download/";
	return path;
}

//returns directory for unfinished downloads
static std::string download_unfinished()
{
	std::string path;
	path += std::getenv("HOME");
	path += "/.p2p/download_unfinished/";
	return path;
}

//returns directory for shared files
static std::string share()
{
	std::string path;
	path += std::getenv("HOME");
	path += "/.p2p/share/";
	return path;
}

}//end namespace path
#endif
