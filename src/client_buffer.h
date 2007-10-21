#ifndef H_CLIENT_BUFFER
#define H_CLIENT_BUFFER

//std
#include <ctime>
#include <list>

#include "download.h"
#include "global.h"

class client_buffer
{
public:
	std::string recv_buff; //buffer for partial recvs
	std::string send_buff; //buffer for partial sends

	client_buffer(const std::string & server_IP_in, int * send_pending_in);

	/*
	add_download       - associate a download with this client_buffer
	empty              - return true if Download is empty
	get_last_seen      - returns the unix timestamp of when this server last responded
	get_IP             - returns the IP address associated with this client_buffer
	post_recv          - does actions which may need to be done after a recv
	post_send          - does actions which may need to be done after a send
	prepare_request    - if another request it needed rotate downloads and get a request
	terminate_download - removes a download_holder from Download which corresponds to hash
	                     returns true if the it can delete the download or if it doesn't exist
	                     returns false if the ClientBuffer is expecting data from the server
	unregister_all     - unregisters this connection with all associated downloads
	                     this needs to be run before deleting this client_buffer
	*/
	void add_download(const unsigned int & file_ID, download * new_download);
	const bool empty();
	const std::string & get_IP();
	const time_t & get_last_seen();
	void post_recv();
	void post_send();
	void prepare_request();
	const bool terminate_download(const std::string & hash);
	void unregister_all();

private:
	//IP associated with this serverElement
	std::string server_IP;

	/*
	Unix timestamp of when the last communication was received from the server
	associated with this client_buffer.
	*/
	time_t last_seen;

	/*
	Set to true if terminate_download was called, this client_buffer has the
	download but it wasn't removed due to it's expecting more bytes. When this
	is set to true it prevents new requests and Download rotation from being
	done.
	*/
	bool terminating;

	//true if client_buffer element is ready to send another request
	bool ready;

	//true if server is not following the protocol, this results in disconnect
	bool abuse;

	//determines whether the client will send, full description on client.h
	int * send_pending;

	unsigned int latest_requested; //what block was most recently requested
	unsigned int bytes_expected;   //how many bytes needed to fulfill request

	class download_holder
	{
	public:
		download_holder(const unsigned int & file_ID_in, download * Download_in)
		{
			file_ID = file_ID_in;
			Download = Download_in;
		}

		unsigned int file_ID;
		download * Download;
	};

	//what Download this socket is currently serving
	std::list<download_holder>::iterator Download_iter;

	//all downloads that this client_buffer is serving
	std::list<download_holder> Download;

	/*
	encode_int       - converts 32bit integer to 4 chars
	rotate_downloads - moves Download_iter through Download in a circular fashion
	*/
	std::string encode_int(const unsigned int & number);
	void rotate_downloads();
};
#endif
