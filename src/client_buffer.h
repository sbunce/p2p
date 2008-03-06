#ifndef H_CLIENT_BUFFER
#define H_CLIENT_BUFFER

//std
#include <ctime>
#include <deque>
#include <list>

//custom
#include "atomic.h"
#include "download.h"
#include "global.h"

class client_buffer
{
public:
	client_buffer(int socket_in, std::string & server_IP_in, atomic<int> * send_pending_in);
	~client_buffer();

	std::string recv_buff; //buffer for partial recvs
	std::string send_buff; //buffer for partial sends

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
	                     returns false if the client_buffer is expecting data from the server
	*/
	void add_download(download * new_download);
	const bool empty();
	const std::string & get_IP();
	const time_t & get_last_seen();
	int post_recv();
	void post_send();
	void prepare_request();
	const bool terminate_download(const std::string & hash);

private:
	std::string server_IP;      //IP associated with this client_buffer
	int socket;                 //socket number of this element
	time_t last_seen;           //used for timeout
	bool abuse;                 //if true a disconnect is triggered
	atomic<int> * send_pending; //must be incremented when send_buff goes from empty to non-empty

	/*
	The Download container is effectively a ring. The rotate_downloads() function will move
	Download_iter through it in a circular fashion.
	*/
	std::list<download *> Download;                //all downloads that this client_buffer is serving
	std::list<download *>::iterator Download_iter; //last download a request was gotten from

	//holds downloads in order of pending response paired with how many bytes are expected
	//std::deque<std::pair<int, download *> > Pipeline;

	class pending_response
	{
	public:
		//the command and how many bytes are associated with it
		std::vector<std::pair<char, int> > expected;

		//the download that made the request
		download * Download;
	};

	std::deque<pending_response> Pipeline;

	/*
	Temporary holder for downloads that need to be terminated. This can be due to the user canceling
	the download or the download completing.
	*/
	std::list<download *> Pending_Termination;

	/*
	rotate_downloads - moves Download_iter through Download in a circular fashion
	unregister_all   - unregisters this connection with all associated downloads
	*/
	void rotate_downloads();
	void unreg_all();

	boost::mutex T_mutex; //mutex for termination function
};
#endif
