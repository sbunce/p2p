#ifndef H_CLIENT_BUFFER
#define H_CLIENT_BUFFER

//boost
#include <boost/thread/mutex.hpp>

//std
#include <ctime>
#include <deque>
#include <list>
#include <set>

//custom
#include "download.h"
#include "global.h"

class client_buffer
{
public:
	client_buffer(const int & socket_in, const std::string & server_IP_in, volatile int * send_pending_in);
	~client_buffer();

	//the client directly accesses these member variables to update them
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

	/*
	Adds a download to the Unique_Download set. This is not called by add_download
	because it's possible a download will be started for which there are no
	active servers.
	*/
	static void add_download_unique(download * new_download)
	{
		boost::mutex::scoped_lock lock(D_mutex);

		Unique_Download.insert(new_download);
	}

	static void remove_download_unique(download * Download)
	{
		boost::mutex::scoped_lock lock(D_mutex);

		Unique_Download.erase(Download);
	}

	//returns information about currently running downloads
	static void current_downloads(std::vector<download_info> & info)
	{
		boost::mutex::scoped_lock lock(D_mutex);

		std::set<download *>::iterator iter_cur, iter_end;
		iter_cur = Unique_Download.begin();
		iter_end = Unique_Download.end();
		while(iter_cur != iter_end){
			download_info Download_Info(
				(*iter_cur)->hash(),
				(*iter_cur)->name(),
				(*iter_cur)->total_size(),
				(*iter_cur)->speed(),
				(*iter_cur)->percent_complete()
			);

			(*iter_cur)->IP_list(Download_Info.server_IP);
			info.push_back(Download_Info);
			++iter_cur;
		}
	}

	//populates a list with hashes of complete downloads
	static void find_complete(std::list<download *> & complete)
	{
		boost::mutex::scoped_lock lock(D_mutex);

		std::set<download *>::iterator iter_cur, iter_end;
		iter_cur = Unique_Download.begin();
		iter_end = Unique_Download.end();
		while(iter_cur != iter_end){
			if((*iter_cur)->complete()){
				complete.push_back(*iter_cur);
			}
			++iter_cur;
		}
	}

	//stops the download associated with hash
	static void stop_download(const std::string & hash)
	{
		boost::mutex::scoped_lock lock(D_mutex);

		std::set<download *>::iterator iter_cur, iter_end;
		iter_cur = Unique_Download.begin();
		iter_end = Unique_Download.end();
		while(iter_cur != iter_end){
			if(hash == (*iter_cur)->hash()){
				(*iter_cur)->stop();
				break;
			}
			++iter_cur;
		}
	}

private:
	//lock for all access to any download (lock for Unique_Download and Download)
	static boost::mutex D_mutex;

	/*
	This contains all downloads known by any client_buffer. Whenever a download
	is added with add_download() it is inserted in this std::set. Because it's a
	set there will be no duplicates.
	*/
	static std::set<download *> Unique_Download;

	/*
	The Download container is effectively a ring. The rotate_downloads() function will move
	Download_iter through it in a circular fashion.
	*/
	std::list<download *> Download;                //all downloads that this client_buffer is serving
	std::list<download *>::iterator Download_iter; //last download a request was gotten from

	std::string server_IP;       //IP associated with this client_buffer
	int socket;                  //socket number of this element
	time_t last_seen;            //used for timeout
	bool abuse;                  //if true a disconnect is triggered
	volatile int * send_pending; //signals client that there is data to send

	class pending_response
	{
	public:
		//the command and how many bytes are associated with it
		std::vector<std::pair<char, int> > expected;

		//the download that made the request
		download * Download;
	};

	/*
	Past requests are stored here. The front of this queue will contain the oldest
	requests. When a response comes in it will correspond to the pending_response
	in the front of this queue.
	*/
	std::deque<pending_response> Pipeline;

	/*
	rotate_downloads - moves Download_iter through Download in a circular fashion
	                   returns true if looped back to beginning
	unregister_all   - unregisters this connection with all associated downloads
	*/
	bool rotate_downloads();
	void unreg_all();
};
#endif
