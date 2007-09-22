#ifndef H_SERVERINDEX
#define H_SERVERINDEX

//boost
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>

#include <string>
#include <sqlite3.h>

#include "global.h"
#include "hash.h"

class serverIndex
{
public:
	serverIndex();
	/*
	is_indexing - true if indexing(scanning for new files and hashing)
	getFileInfo - sets fileSize and filePath based on file_ID
	            - returns true if file found, false if file not found
	start       - starts indexShare_thread
	*/
	bool is_indexing();
	bool getFileInfo(const int & file_ID, int & fileSize, std::string & filePath);
	void start();

private:
	bool indexing;

	/*
	Wrappers for call-back functions. These exist to convert the void pointer
	that sqlite3_exec allows as a call-back in to a member function pointer that
	is needed to call a member function of *this class.
	*/
	static int addEntry_callBack_wrapper(void * objectPtr, int columnsRetrieved, char ** queryResponse, char ** columnName)
	{
		serverIndex * thisClass = (serverIndex *)objectPtr;
		thisClass->addEntry_callBack(columnsRetrieved, queryResponse, columnName);
		return 0;
	}
	static int getFileInfo_callBack_wrapper(void * objectPtr, int columnsRetrieved, char ** queryResponse, char ** columnName)
	{
		serverIndex * thisClass = (serverIndex *)objectPtr;
		thisClass->getFileInfo_callBack(columnsRetrieved, queryResponse, columnName);
		return 0;
	}
	static int removeMissing_callBack_wrapper(void * objectPtr, int columnsRetrieved, char ** queryResponse, char ** columnName)
	{
		serverIndex * thisClass = (serverIndex *)objectPtr;
		thisClass->removeMissing_callBack(columnsRetrieved, queryResponse, columnName);
		return 0;
	}

	/*
	The call-back functions themselves. These are called by the wrappers which are
	passed to sqlite3_exec. These contain the logic of what needs to be done.
	*/
	void addEntry_callBack(int & columnsRetrieved, char ** queryResponse, char ** columnName);
	void getFileInfo_callBack(int & columnsRetrieved, char ** queryResponse, char ** columnName);
	void removeMissing_callBack(int & columnsRetrieved, char ** queryResponse, char ** columnName);

	/*
	These variables are used by the call-back functions to communicate back with
	their caller functions.
	*/
	bool addEntry_entryExists;
	bool getFileInfo_entryExists;
	int getFileInfo_fileSize;
	std::string getFileInfo_filePath;

	sqlite3 * sqlite3_DB; //sqlite database pointer to be passed to functions like sqlite3_exec
	hash Hash;            //hash generator for blocks and whole files

	/*
	addEntry           - adds an entry to the database if none exists for the file
	indexShare         - removes files listed in index that don't exist in share
	                   - adds files that exist in share but no in index
	                   - checks the share every global::SERVER_REFRESH seconds
	indexShare_recurse - recursive function to locate all files in share(calls addEntry)
	removeMissing      - removes files from the database that aren't present
	*/
	void addEntry(const int & size, const std::string & path);
	void indexShare_thread();
	int indexShare_recurse(const std::string directoryName);
	void removeMissing();
};
#endif

