#include <iostream>
#include <sstream>

#include "DB_access.h"

DB_access::DB_access()
{
	int return_code;
	if((return_code = sqlite3_open(global::DATABASE_PATH.c_str(), &sqlite3_DB)) != 0){
#ifdef DEBUG
		std::cout << "error: DB_access::DB_access() #1 failed with sqlite3 error " << return_code << "\n";
#endif
	}
}

bool DB_access::file_info(const unsigned int & file_ID, unsigned long & file_size, std::string & file_path)
{
	std::ostringstream query;
	file_info_entry_exists = false;

	//locate the record
	query << "SELECT size, path FROM share WHERE ID LIKE \"" << file_ID << "\" LIMIT 1";

	int return_code;
	if((return_code = sqlite3_exec(sqlite3_DB, query.str().c_str(), file_info_call_back_wrapper, (void *)this, NULL)) != 0){
#ifdef DEBUG
		std::cout << "error: server_index::file_info() failed with sqlite3 error " << return_code << "\n";
#endif
	}

	if(file_info_entry_exists){
		file_size = file_info_file_size;
		file_path = file_info_file_path;
		return true;
	}
	else{
		return false;
	}
}

void DB_access::file_info_call_back(int & columns_retrieved, char ** query_response, char ** column_name)
{
	file_info_entry_exists = true;
	file_info_file_size = strtoul(query_response[0], NULL, 0);
	file_info_file_path.assign(query_response[1]);
}
