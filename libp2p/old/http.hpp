//THREADSAFE
#ifndef H_HTTP
#define H_HTTP

//custom
#include "database.hpp"
#include "settings.hpp"

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
	database::connection DB;

	/*
	table_table - adds html table to send for database table
	*/
	void table_table(const std::string & table_name, std::string & send);
};
#endif
