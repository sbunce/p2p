#ifndef H_DB_DOWNLOAD
#define H_DB_DOWNLOAD

//boost
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/tuple/tuple.hpp>

//custom
#include "download_info.h"
#include "global.h"
#include "hash_tree.h"
#include "sha.h"

//sqlite
#include <sqlite3.h>

//std
#include <fstream>
#include <list>
#include <sstream>
#include <string>
#include <vector>

class DB_download
{
public:
	DB_download();

	/*
	Functions to modify the database:
	start_download     - adds a download to the database if it doesn't already exist
	                     returns true if download could be added, else false
	terminate_download - removes the download entry from the database
	*/
	bool start_download(const download_info & info);
	void terminate_download(const std::string & hash);

	/*
	Lookup functions:
	lookup_<*>  - look up information by <*>
	resume      - fill the vector with download information (used to resume downloads on program start)
	*/
	bool lookup_hash(const std::string & hash, boost::uint64_t & file_size);
	bool lookup_hash(const std::string & hash, std::string & path);
	bool lookup_hash(const std::string & hash, std::string & path, boost::uint64_t & file_size);
	void resume(std::vector<download_info> & resume_DL);

private:
	sqlite3_wrapper DB;

	/*
	is_downloading   - returns true if hash is found in download table, else false
	resume_call_back - call back for resume()
	*/
	bool is_downloading(const std::string & hash);
	int resume_call_back(std::vector<download_info> & resume, int columns_retrieved, char ** response, char ** column_name);
};
#endif
