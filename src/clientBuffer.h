#ifndef H_CLIENT_BUFFER
#define H_CLIENT_BUFFER

#include <list>

#include "download.h"
#include "global.h"

class clientBuffer
{
public:
	std::string recvBuffer; //buffer for partial recvs
	std::string sendBuffer; //buffer for partial sends

	//true if clientBuffer element is ready to send another request
	bool ready;

	//true if server is not following the protocol, this results in disconnect
	bool abusive;

	clientBuffer(const std::string & server_IP_in, int * sendPending_in);

	class downloadHolder
	{
	public:
		downloadHolder(const unsigned int & file_ID_in, download * Download_in)
		{
			file_ID = file_ID_in;
			Download = Download_in;
		}

		unsigned int file_ID;
		download * Download;
	};

	//what Download this socket is currently serving
	std::list<downloadHolder>::iterator Download_iter;
	std::list<downloadHolder> Download;

	/*
	addDownload    - associate a download with this clientBuffer
	get_IP         - returns the IP address associated with this clientBuffer
	postRecv       - does actions which may need to be done after a recv
	postSend       - does actions which may need to be done after a send
	prepareRequest - if another request it needed rotate downloads and get a request
	*/
	void addDownload(const unsigned int & file_ID, download * newDownload);
	const std::string & get_IP();
	void postRecv();
	void postSend();
	void prepareRequest();

private:
	//IP associated with this serverElement
	std::string server_IP;

	//determines whether the client will send, full description on client.h
	int * sendPending;

	unsigned int latestRequested; //what block was most recently requested
	unsigned int bytesExpected;   //how many bytes needed to fulfill request

	/*
	encodeInt       - converts 32bit integer to 4 chars
	rotateDownloads - moves Download_iter through Download in a circular fashion
	*/
	std::string encodeInt(const unsigned int & number);
	void rotateDownloads();
};
#endif
