#ifndef H_DB_DOWNLOAD
#define H_DB_DOWNLOAD

//boost
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/tuple/tuple.hpp>

//custom
#include "DB_hash.h"
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
	complete  - removes the download entry
	start     - adds a download to the database if it doesn't already exist
	            returns true and sets key if download could be added
	terminate - removes the download entry and tree
	*/
	void complete(const std::string & hash);
	bool start(download_info & info);
	void terminate(const std::string & hash);

	/*
	Lookup functions:
	lookup_<*>  - look up information by <*>
	resume      - fill the vector with download information (used to resume downloads on program start)
	*/
	bool lookup_hash(const std::string & hash);
	bool lookup_hash(const std::string & hash, boost::int64_t & key);
	bool lookup_hash(const std::string & hash, std::string & path);
	bool lookup_hash(const std::string & hash, std::string & path, boost::uint64_t & file_size);
	void resume(std::vector<download_info> & resume_DL);

private:
	/*
	is_downloading   - returns true if hash is found in download table, else false
	resume_call_back - call back for resume()
	*/
	bool is_downloading(const std::string & hash);
	int resume_call_back(std::vector<download_info> & resume, int columns_retrieved, char ** response, char ** column_name);

	sqlite3_wrapper::database DB;
	DB_hash DB_Hash;
};
#endif
