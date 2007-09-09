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
	class infoBuffer
	{
	public:
		infoBuffer()
		{
			//defaults which don't need to be set on new downloads
			resumed = false;
			latestRequest = 0;
			currentSuperBlock = 0;
		}

		bool resumed; //if this infoBuffer is being used to resume a download

		std::string hash;
		std::string fileName;
		std::string fileSize;
		unsigned int latestRequest; //what block was most recently requested
		int currentSuperBlock;

		//information unique to the server, these vectors are parallel
		std::vector<std::string> server_IP;
		std::vector<std::string> file_ID;
	};

	exploration();
	/*
	search - searches the database for names which match searchWord
	*/
	void search(std::string & searchWord, std::list<infoBuffer> & info);

private:
	/*
	Wrappers for call-back functions. These exist to convert the void pointer
	that sqlite3_exec allows as a call-back in to a member function pointer that
	is needed to call a member function of *this class.
	*/
	static int search_callBack_wrapper(void * objectPtr, int columnsRetrieved, char ** queryResponse, char ** columnName)
	{
		exploration * thisClass = (exploration *)objectPtr;
		thisClass->search_callBack(columnsRetrieved, queryResponse, columnName);
		return 0;
	}

	/*
	The call-back functions themselves. These are called by the wrappers which are
	passed to sqlite3_exec. These contain the logic of what needs to be done.
	*/
	void search_callBack(int & columnsRetrieved, char ** queryResponse, char ** columnName);

	//contains the results from the last search
	std::list<infoBuffer> searchResults;

	//sqlite database pointer to be passed to functions like sqlite3_exec
	sqlite3 * sqlite3_DB;

	/*
	percentToAsterisk - converts all '*' in a string to '%'
	*/
	void asteriskToPercent(std::string & searchWord);
};
#endif
