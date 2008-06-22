#ifndef H_DB_SEARCH
#define H_DB_SEARCH

//custom
#include "download_info.h"

//sqlite
#include <sqlite3.h>

//std
#include <string>

class DB_search
{
public:
	DB_search();

	/*
	search      - searches the database for names which match search_word
	get_servers - populates the servers vector with IP addresses associated with hash
	*/
	void search(std::string & search_word, std::vector<download_info> & info);
	void get_servers(const std::string & hash, std::vector<std::string> & servers);

private:
	sqlite3 * sqlite3_DB;
	boost::mutex Mutex;   //mutex for all public functions

	void search_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	static int search_call_back_wrapper(void * obj_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		DB_search * this_class = (DB_search *)obj_ptr;
		this_class->search_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}
	std::vector<download_info> * search_results_ptr;

	void get_servers_call_back(int & columns_retrieved, char ** query_response, char ** column_name);
	static int get_servers_call_back_wrapper(void * obj_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		DB_search * this_class = (DB_search *)obj_ptr;
		this_class->get_servers_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}
	std::vector<std::string> * get_servers_results_ptr;
};
#endif
