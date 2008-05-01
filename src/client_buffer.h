#ifndef H_CLIENT_BUFFER
#define H_CLIENT_BUFFER

//std
#include <ctime>
#include <deque>
#include <list>

//custom
#include "download.h"
#include "global.h"

class client_buffer
{
public:
	client_buffer(const int & socket_in, const std::string & server_IP_in, volatile int * send_pending_in);
	~client_buffer();

	std::string recv_buff; //buffer for partial recvs
	std::string send_buff; //buffer for partial sends

	/*
	add_download       - associate a download with this client_buffer
	empty              - return true if Download is empty
	get_IP             - returns the IP address associated with this client_buffer
	get_last_seen      - returns the unix timestamp of when this server last responded
	post_recv          - does actions which may need to be done after a recv
	post_send          - does actions which may need to be done after a send
	prepare_request    - if another request it needed rotate downloads and get a request
	terminate_download - removes the download which corresponds to hash
	*/
	void add_download(download * new_download);
	bool empty();
	const std::string & get_IP();
	const time_t & get_last_seen();
	void post_recv();
	void post_send();
	void prepare_request();
	void terminate_download(download * term_DL);

private:
	std::string server_IP;       //IP associated with this client_buffer
	int socket;                  //socket number of this element
	time_t last_seen;            //used for timeout
	bool abuse;                  //if true a disconnect is triggered
	volatile int * send_pending; //signals client that there is data to send

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
	rotate_downloads - moves Download_iter through Download in a circular fashion
	                   returns true if looped back to beginning
	unregister_all   - unregisters this connection with all associated downloads
	*/
	bool rotate_downloads();
	void unreg_all();

	boost::mutex T_mutex; //mutex for termination function
};
#endif
