#ifndef H_CLIENT_INDEX
#define H_CLIENT_INDEX

//boost
#include <boost/thread/mutex.hpp>

//std
#include <string>
#include <vector>

#include "clientBuffer.h"
#include "globals.h"

class clientIndex
{
public:
	clientIndex();
	/*
	downloadComplete  - removes the download entry from the download index
	getFilePath       - gets the filePath based on the server_IP and file_ID
	                  - precondition - make sure the download was started!
	initialFillBuffer - returns the necessary elements to fill the client sendBuffer
	startDownload     - returns false if the download is already in progress

	download.index format
	<server ip>:<file_ID>:<fileSize>:<fileName>
	example:
	127.0.0.1:2:0:39088132:some movie.mpeg
	*/
	void downloadComplete(std::string messageDigest_in);
	std::string getFilePath(std::string messageDigest_in);
	void initialFillBuffer(std::vector<clientBuffer> & sendBuffer);
	int startDownload(std::string messageDigest, std::string server_IP, std::string file_ID, std::string fileSize, std::string fileName);

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
