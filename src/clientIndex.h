#ifndef H_CLIENT_INDEX
#define H_CLIENT_INDEX

//boost
#include <boost/thread/mutex.hpp>

//std
#include <string>
#include <vector>

#include "clientBuffer.h"
#include "exploration.h"
#include "globals.h"

class clientIndex
{
public:
	clientIndex();
	/*
	terminateDownload  - removes the download entry from the download index
	getFilePath        - gets the filePath based on the server_IP and file_ID
	                   - precondition - make sure the download was started!
	initialFillBuffer  - returns the necessary elements to fill the client sendBuffer
	startDownload      - returns false if the download is already in progress

	download.index format
	<messageDigest>:<fileName>:<fileSize>:<server info>
	<server info> = <server IP>:<file_ID>:<server info>
	<server info> = <server IP>:<file_ID>
	*/
	void terminateDownload(std::string messageDigest_in);
	std::string getFilePath(std::string messageDigest_in);
	void initialFillBuffer(std::vector<clientBuffer> & sendBuffer);
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
