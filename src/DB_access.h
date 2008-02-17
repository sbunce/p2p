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
	file_info - sets file_size and file_path based on file_ID
	          - returns true if file found
	*/
	bool file_info(const unsigned int & file_ID, unsigned long & file_size, std::string & file_path);

private:
	//pointer for all DB accesses
	sqlite3 * sqlite3_DB;

	/*
	call back function
	call back wrapper
	variables associated with call back
	*/

	//call back used by file_info
	void file_info_call_back(int & columns_retrieved, char ** query_response, char ** column_name);

	static int file_info_call_back_wrapper(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		DB_access * this_class = (DB_access *)object_ptr;
		this_class->file_info_call_back(columns_retrieved, query_response, column_name);
		return 0;
	}

	unsigned long file_info_file_size;
	std::string file_info_file_path;
	bool file_info_entry_exists;


};
#endif
