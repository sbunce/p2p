#ifndef H_HTTP
#define H_HTTP

//boost
#include <boost/thread/mutex.hpp>

//custom
#include "DB_blacklist.h"
#include "DB_client_preferences.h"
#include "DB_download.h"
#include "DB_hash_free.h"
#include "DB_prime.h"
#include "DB_search.h"
#include "DB_server_preferences.h"
#include "DB_share.h"
#include "global.h"

//std
#include <string>

class http
{
public:
	http();

	/*
	process - process a HTTP request
	          request: HTTP request
	          send:    response to send
	*/
	void process(const std::string & request, std::string & send);

private:
	sqlite3 * sqlite3_DB;
	boost::mutex Mutex;   //mutex for all public functions

	static int table_column_name_call_back(void * obj_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		std::string * send = (std::string *)obj_ptr;
		"\t<b><tr>\n";
		for(int x=0; x<columns_retrieved; ++x){
			*send += "\t<td>";
			*send += column_name[x];
			*send += "</td>\n";
		}
		*send += "\t</tr></b>\n";
		return 0;
	}

	static int table_data_call_back(void * obj_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		std::string * send = (std::string *)obj_ptr;
		"\t<tr>\n";
		for(int x=0; x<columns_retrieved; ++x){
			*send += "\t<td>";
			*send += query_response[x];
			*send += "</td>\n";
		}
		*send += "\t</tr>\n";
		return 0;
	}

	/*
	table_table - adds html table to send for database table
	*/
	void table_table(const std::string & table_name, std::string & send);
};
#endif
