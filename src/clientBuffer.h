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

	clientBuffer(const std::string & server_IP_in, int * sendPending_in);

	/*
	addDownload       - associate a download with this clientBuffer
	empty             - return true if Download is empty
	get_IP            - returns the IP address associated with this clientBuffer
	postRecv          - does actions which may need to be done after a recv
	postSend          - does actions which may need to be done after a send
	prepareRequest    - if another request it needed rotate downloads and get a request
	terminateDownload - removes a downloadHolder from Download which corresponds to hash
	                    returns true if the it can delete the download or if it doesn't exist
	                    returns false if the ClientBuffer is expecting data from the server
	unregisterAll     - unregisters this connection with all associated downloads
	                    this needs to be run before deleting this clientBuffer
	*/
	void addDownload(const unsigned int & file_ID, download * newDownload);
	const bool empty();
	const std::string & get_IP();
	void postRecv();
	void postSend();
	void prepareRequest();
	const bool terminateDownload(const std::string & hash);
	void unregisterAll();

private:
	//IP associated with this serverElement
	std::string server_IP;

	/*
	Set to true if terminateDownload was called, this clientBuffer has the
	download but it wasn't removed due to it's expecting more bytes. When this
	is set to true it prevents new requests and Download rotation from being
	done.
	*/
	bool terminating;

	//true if clientBuffer element is ready to send another request
	bool ready;

	//true if server is not following the protocol, this results in disconnect
	bool abuse;

	//determines whether the client will send, full description on client.h
	int * sendPending;

	unsigned int latestRequested; //what block was most recently requested
	unsigned int bytesExpected;   //how many bytes needed to fulfill request

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
	encodeInt       - converts 32bit integer to 4 chars
	rotateDownloads - moves Download_iter through Download in a circular fashion
	*/
	std::string encodeInt(const unsigned int & number);
	void rotateDownloads();
};
#endif
