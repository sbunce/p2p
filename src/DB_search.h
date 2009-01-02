#ifndef H_DB_SEARCH
#define H_DB_SEARCH

//custom
#include "download_info.h"
#include "sqlite3_wrapper.h"

//std
#include <algorithm>
#include <sstream>
#include <string>

class DB_search
{
public:
	DB_search();

	/*
	search      - searches the database for names which match search_word
	get_servers - populates the servers vector with IP addresses associated with hash
	*/
	void search(std::string & search_term, std::vector<download_info> & info);
	void get_servers(const std::string & hash, std::vector<std::string> & servers);

private:
	sqlite3_wrapper DB;
};
#endif
