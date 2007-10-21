#ifndef H_EXPLORATION
#define H_EXPLORATION

//std
#include <list>
#include <sqlite3.h>
#include <string>

#include "global.h"

class exploration
{
public:
	//used to pass information to user interface
	class info_buffer
	{
	public:
		info_buffer()
		{
			//defaults which don't need to be set on new downloads
			resumed = false;
			latest_request = 0;
			current_super_block = 0;
		}

		bool resumed; //if this info_buffer is being used to resume a download

		std::string hash;
		std::string file_name;
		std::string file_size;
		unsigned int latest_request; //what block was most recently requested
		int current_super_block;

		//information unique to the server, these vectors are parallel
		std::vector<std::string> server_IP;
		std::vector<std::string> file_ID;
	};

	exploration();
	/*
	search - searches the database for names which match search_word
	*/
	void search(std::string & search_word, std::list<info_buffer> & info);

private:
	/*
	Wrappers for call-back functions. These exist to convert the void pointer
	that sqlite3_exec allows as a call-back in to a member function pointer that
	is needed to call a member function of *this class.
	*/
	static int search_callBack_wrapper(void * obj_ptr, int columns_retrieved, char ** query_response, char ** column_name)
	{
		exploration * this_class = (exploration *)obj_ptr;
		this_class->search_callBack(columns_retrieved, query_response, column_name);
		return 0;
	}

	/*
	The call-back functions themselves. These are called by the wrappers which are
	passed to sqlite3_exec. These contain the logic of what needs to be done.
	*/
	void search_callBack(int & columns_retrieved, char ** query_response, char ** column_name);

	//contains the results from the last search
	std::list<info_buffer> Search_Results;

	//sqlite database pointer to be passed to functions like sqlite3_exec
	sqlite3 * sqlite3_DB;

	/*
	percentToAsterisk - converts all '*' in a string to '%'
	*/
	void asteriskToPercent(std::string & search_word);
};
#endif
