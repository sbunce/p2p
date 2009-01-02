#ifndef H_HTTP
#define H_HTTP

//custom
#include "global.h"
#include "sqlite3_wrapper.h"

//sqlite
#include <sqlite3.h>

//std
#include <algorithm>
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
	sqlite3_wrapper DB;

	/*
	table_table - adds html table to send for database table
	*/
	void table_table(const std::string & table_name, std::string & send);
};
#endif
