#ifndef H_P2P_BUFFER
#define H_P2P_BUFFER

//boost
#include <boost/thread.hpp>

//custom
#include "download.hpp"
#include "download_status.hpp"
#include "upload_status.hpp"

//std
#include <map>

class p2p_buffer : private boost::noncopyable
{
public:
	p2p_buffer();

	/* General

	*/

	/* Download Related
	current_download:
		Retrieves status of download with specified hash. Returns true and sets
		status if download found. Returns false if download not running.
	current_downloads:
		Populates status vector with all running download information.
	*/
	bool current_download(const std::string & hash, download_status & status);
	void current_downloads(std::vector<download_status> & status);

	/* Upload Related
	is_downloading:
		Returns true if download running.
	*/
	void current_uploads(std::vector<upload_status> & status);
	bool is_downloading(const std::string & hash);

private:
	//mutex to lock all public functions
	boost::mutex Mutex;

	class socket_state
	{
	public:
		socket_state():
			send_pending(0)
		{}

		/*
		How many buffers are non-empty. Used to determine whether to check sockets
		to see if they can be written to. Checking for writeableness is CPU intensive.
		*/
		int send_pending;
	};
	std::map<int, socket_state> Socket_State;

	/*
	Currently running downloads. The download is the state needed by all the
	different download network states. These are kept unique.
	*/
	std::map<std::string, boost::shared_ptr<download> > Running_Download;

	/*
	erase:
		Remove the socket_state associated with socket_FD.
	*/
	void erase(const int & socket_FD);


};
#endif
