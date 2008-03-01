#ifndef H_DB_ACCESS
#define H_DB_ACCESS

#include <string>
#include <sqlite3.h>

#include "global.h"

class DB_access
{
public:
	DB_access();

	/*
	Prefixes denote what table will be read/written by the function. For example
	a share_ function deals with the share table.

	share_file_info - sets file_size and file_path based on file_ID
	                - returns true if file found
	*/
	bool share_file_info(const unsigned int & file_ID, unsigned long & file_size, std::string & file_path);

private:
	//pointer for all DB accesses
	sqlite3 * sqlite3_DB;

	/*
	call back function
	call back wrapper
	variables associated with call back
	*/

	//call back used by share_file_info
	void share_file_info_call_back(int & columns_retrieved, char ** query_response, char ** column_name);

	static int share_file_info_call_back_wrapper(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		DB_access * this_class = (DB_access *)object_ptr;
		this_class->share_file_info_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}

	unsigned long share_file_info_file_size;
	std::string share_file_info_file_path;
	bool share_file_info_entry_exists;


};
#endif
