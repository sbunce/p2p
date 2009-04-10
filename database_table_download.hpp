//THREADSAFE
#ifndef H_DATABASE_TABLE_DOWNLOAD
#define H_DATABASE_TABLE_DOWNLOAD

//boost
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/tokenizer.hpp>
#include <boost/tuple/tuple.hpp>

//custom
#include "database_table_hash.hpp"
#include "download_info.hpp"
#include "hash_tree.hpp"
#include "path.hpp"
#include "settings.hpp"

//include
#include <database_connection.hpp>
#include <sha.hpp>

//std
#include <fstream>
#include <list>
#include <sstream>
#include <string>
#include <vector>

namespace database{
namespace table{
class download
{
public:
	download();

	/*
	complete  - removes the download entry
	start     - adds a download to the database if it doesn't already exist
	            returns true and sets key if download could be added
	terminate - removes the download entry and tree
	*/
	void complete(const std::string & hash, const int & file_size);
	static void complete(const std::string & hash, const int & file_size, database::connection & DB);
	bool start(download_info & info);
	static bool start(download_info & info, database::connection & DB);
	void terminate(const std::string & hash, const int & file_size);
	static void terminate(const std::string & hash, const int & file_size, database::connection & DB);

	/*
	Lookup functions:
	lookup_<*>  - look up information by <*>
	resume      - fill the vector with download information (used to resume downloads on program start)
	*/
	bool lookup_hash(const std::string & hash);
	static bool lookup_hash(const std::string & hash, database::connection & DB);
	bool lookup_hash(const std::string & hash, std::string & path);
	static bool lookup_hash(const std::string & hash, std::string & path, database::connection & DB);
	bool lookup_hash(const std::string & hash, boost::uint64_t & size);
	static bool lookup_hash(const std::string & hash, boost::uint64_t & size, database::connection & DB);
	bool lookup_hash(const std::string & hash, std::string & path, boost::uint64_t & file_size);
	static bool lookup_hash(const std::string & hash, std::string & path, boost::uint64_t & file_size, database::connection & DB);
	void resume(std::vector<download_info> & resume_DL);
	static void resume(std::vector<download_info> & resume_DL, database::connection & DB);

private:
	database::connection DB;

	/*
	is_downloading   - returns true if hash is found in download table, else false
	resume_call_back - call back for resume()
	*/
	static bool is_downloading(const std::string & hash, database::connection & DB);
};
}//end of table namespace
}//end of database namespace
#endif
