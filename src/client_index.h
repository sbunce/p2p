#ifndef H_CLIENT_INDEX
#define H_CLIENT_INDEX

//std
#include <list>
#include <string>
#include <sqlite3.h>
#include <vector>

#include "download.h"
#include "exploration.h"
#include "global.h"

class client_index
{
public:
	client_index();
	/*
	getFilepath       - sets path that corresponds to hash
	                  - returns true if record found(it should be otherwise there is a logic problem)
	initial_fill_buff - restarts downloads on program start that havn't yet finished
	                  - sets up server rings in the serverHolder and download.Server's
	start_download     - adds a download to the database
	terminate_download - removes the download entry from the database
	*/
	bool get_file_path(const std::string & hash, std::string & path);
	void initial_fill_buff(std::list<exploration::info_buffer> & resumed_download);
	bool start_download(exploration::info_buffer & info);
	void terminate_download(const std::string & hash);

private:
	/*
	Wrappers for call-back functions. These exist to convert the void pointer
	that sqlite3_exec allows as a call-back in to a member function pointer that
	is needed to call a member function of *this class.
	*/
	static int get_file_path_callBack_wrapper(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		client_index * this_class = (client_index *)object_ptr;
		this_class->get_file_path_callBack(columns_retrieved, query_response, column_name);
		return 0;
	}

	static int initial_fill_buff_callBack_wrapper(void * object_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		client_index * this_class = (client_index *)object_ptr;
		this_class->initial_fill_buff_callBack(columns_retrieved, query_response, column_name);
		return 0;
	}

	/*
	The call-back functions themselves. These are called by the wrappers which are
	passed to sqlite3_exec. These contain the logic of what needs to be done.
	*/
	void get_file_path_callBack(int & columns_retrieved, char ** query_response, char ** column_name);
	void initial_fill_buff_callBack(int & columns_retrieved, char ** query_response, char ** column_name);

	/*
	These variables are used by the call-back functions to communicate back with
	their caller functions.
	*/
	bool get_file_path_entry_exits;
	std::string get_file_path_file_name;
	std::list<exploration::info_buffer> initial_fill_buff_resumed_download;

	sqlite3 * sqlite3_DB; //sqlite database pointer to be passed to functions like sqlite3_exec
};
#endif
