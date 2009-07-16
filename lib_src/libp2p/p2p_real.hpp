//THREADSAFE, CTOR THREAD SPAWNING
#ifndef H_P2P_REAL
#define H_P2P_REAL

//custom
#include "database.hpp"
#include "hash_tree.hpp"
#include "number_generator.hpp"
#include "path.hpp"
#include "settings.hpp"
#include "share.hpp"

//include
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>
#include <p2p/p2p.hpp>

//standard
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

class p2p_real : private boost::noncopyable
{
public:
	p2p_real();
	~p2p_real();

	/*
	current_downloads:
		Returns info for all downloads.
	current_uploads:
		Populates info with upload_info for all uploads.
	download_rate:
		Returns average download rate for everything (excluding localhost).
	get_max_connections:
		Returns connection limit.
	get_download_directory:
		Returns the location where downloads are saved to.
	get_max_download_rate:
		Returns the download rate limit.
	pause_download:
		Pause a download.
	prime_count:
		Returns size of prime cache used for key-exchanges.
	remove_download:
		Stops/Removes a running download.
	set_max_connections:
		Sets connection limit. Disconnects servers at random if over the limit.
	set_download_directory:
		Set location where downloads are to be saved to. Does not change location
		existing downloads are to be saved to.
	set_max_download_rate:
		Sets the upload rate limit.
	share_size:
		Size of all shared files. (bytes)
	start_download:
		Starts a download.
	upload_rate:
		Returns average upload rate for everything (excluding localhost).
	*/
	void current_downloads(std::vector<download_status> & CD);
	void current_uploads(std::vector<upload_status> & CU);
	unsigned download_rate();
	unsigned get_max_connections();
	unsigned get_max_download_rate();
	unsigned get_max_upload_rate();
	void pause_download(const std::string & hash);
	unsigned prime_count();
	void remove_download(const std::string & hash);
	void set_max_connections(const unsigned max_connections);
	void set_max_download_rate(const unsigned max_download_rate);
	void set_max_upload_rate(const unsigned max_upload_rate);
	boost::uint64_t share_size();
	void start_download(const download_info & DI);
	unsigned upload_rate();

private:
	/*
	reconnect_unfinished:
		Resumes downloads upon program start.
	*/
	void reconnect_unfinished();
};
#endif
