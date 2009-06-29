//THREADSAFE
#ifndef H_DATABASE_TABLE_SEARCH
#define H_DATABASE_TABLE_SEARCH

//custom
#include "database.hpp"
#include "path.hpp"

//include
#include <boost/tokenizer.hpp>
#include <p2p/download_info.hpp>

//std
#include <algorithm>
#include <sstream>
#include <string>

namespace database{
namespace table{
class search
{
public:
	search();

	/*
	search      - searches the database for names which match search_word
	get_servers - populates the servers vector with IP addresses associated with hash
	*/
	void run_search(std::string & search_term, std::vector<download_info> & search_results);
	static void run_search(std::string & search_term, std::vector<download_info> & search_results, database::connection & DB);
	void get_servers(const std::string & hash, std::vector<std::string> & servers);
	static void get_servers(const std::string & hash, std::vector<std::string> & servers, database::connection & DB);

private:
	database::connection DB;
};
}//end of namespace table
}//end of namespace database
#endif
