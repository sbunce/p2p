#ifndef H_CLIENT_INDEX
#define H_CLIENT_INDEX

//boost
#include <boost/thread/mutex.hpp>

//std
#include <string>
#include <vector>

#include "download.h"
#include "exploration.h"
#include "globals.h"

class clientIndex
{
public:
	clientIndex();
	/*
	terminateDownload - removes the download entry from the download index
	getFilePath       - gets the filePath based on the server_IP and file_ID
	                  - precondition - make sure the download was started!
	initialFillBuffer - fills the serverHolder and downloadBuffer on resume, returns true if elements added
	startDownload     - returns false if the download is already in progress

	download.index format
	<messageDigest>:<fileName>:<fileSize>:<server info>
	<server info> = <server IP>:<file_ID>:<server info>
	<server info> = <server IP>:<file_ID>
	*/
	void terminateDownload(std::string messageDigest_in);
	std::string getFilePath(std::string messageDigest_in);
	bool initialFillBuffer(std::list<download> & downloadBuffer, std::list<std::list<download::serverElement *> > & serverHolder);
	int startDownload(exploration::infoBuffer info);

private:
	std::string indexName;         //the name of the download index file
	std::string downloadDirectory; //the name of the download directory

	/*
	Although there is no threading within this class, this class will be getting
	called by different threads.
	*/
	boost::mutex Mutex;
};
#endif
