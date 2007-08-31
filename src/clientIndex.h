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

class clientIndex
{
public:
	clientIndex();
	/*
	getFilepath       - sets path that corresponds to hash
	                  - returns true if record found(it should be otherwise there is a logic problem)
	initialFillBuffer - fills the client downloadBuffer after client is first started
	                  - sets up server rings in the serverHolder and download.Server's
	startDownload     - adds a download to the database
	terminateDownload - removes the download entry from the database
	*/
	bool getFilePath(const std::string & hash, std::string & path);


//needs to be reworked
	//void initialFillBuffer(std::list<download> & downloadBuffer, std::list<std::list<download::serverElement *> > & serverHolder);


	bool startDownload(exploration::infoBuffer & info);
	void terminateDownload(const std::string & hash);

private:

	/*
	Wrappers for call-back functions. These exist to convert the void pointer
	that sqlite3_exec allows as a call-back in to a member function pointer that
	is needed to call a member function of *this class.
	*/
	static int getFilePath_callBack_wrapper(void * objectPtr, int columnsRetrieved, char ** queryResponse, char ** columnName)
	{
		clientIndex * thisClass = (clientIndex *)objectPtr;
		thisClass->getFilePath_callBack(columnsRetrieved, queryResponse, columnName);
		return 0;
	}

/*
	static int initialFillBuffer_callBack_wrapper(void * objectPtr, int columnsRetrieved, char ** queryResponse, char ** columnName)
	{
		clientIndex * thisClass = (clientIndex *)objectPtr;
		thisClass->initialFillBuffer_callBack(columnsRetrieved, queryResponse, columnName);
		return 0;
	}
*/
	/*
	The call-back functions themselves. These are called by the wrappers which are
	passed to sqlite3_exec. These contain the logic of what needs to be done.
	*/
	void getFilePath_callBack(int & columnsRetrieved, char ** queryResponse, char ** columnName);


//needs to be reworked
	//void initialFillBuffer_callBack(int & columnsRetrieved, char ** queryResponse, char ** columnName);

	/*
	These variables are used by the call-back functions to communicate back with
	their caller functions.
	*/
	bool getFilePath_entryExists;
	std::string getFilePath_fileName;
	std::list<download> initialFillBuffer_downloadBuffer;
//	std::list<std::list<download::serverElement *> > initialFillBuffer_serverHolder;

	sqlite3 * sqlite3_DB; //sqlite database pointer to be passed to functions like sqlite3_exec
};
#endif
