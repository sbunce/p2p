#ifndef H_LOAD_SCANNER
#define H_LOAD_SCANNER

//custom
#include "path.hpp"

//include
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/utility.hpp>
#include <convert.hpp>
#include <thread_pool.hpp>
#include <logger.hpp>
#include <SHA1.hpp>
#include <boost/algorithm/string.hpp>

//standard
#include <fstream>
#include <set>
#include <string>

class load_scanner : private boost::noncopyable
{
public:
	load_scanner();
	~load_scanner();

private:
	/*
	load:
		Load file at path.
	scan:
		Scans load directory.
	start:
		Start download.
	*/
	void load(const boost::filesystem::path & load_file);
	void scan();
	void start(const std::string & hash, const std::string file_name);

	thread_pool Thread_Pool;
};
#endif
