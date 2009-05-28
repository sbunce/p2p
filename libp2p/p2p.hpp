//THREADSAFE, CTOR THREAD SPAWNING
#ifndef H_P2P
#define H_P2P

//boost
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

//custom
#include "database.hpp"
#include "download.hpp"
#include "download_info.hpp"
#include "download_status.hpp"
#include "hash_tree.hpp"
#include "number_generator.hpp"
#include "path.hpp"
#include "settings.hpp"
#include "share_index.hpp"
#include "upload_status.hpp"

//include
#include <atomic_int.hpp>

//net
//#include <network.hpp>

//std
#include <cstdlib>
#include <ctime>
#include <deque>
#include <fstream>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <vector>

class p2p : private boost::noncopyable
{
public:
	p2p();
	~p2p();

	/* Public Interface
	current_download:
		Returns info for one download that corresponds to hash. Returns true and
		sets info if download found.
	current_downloads:
		Returns info for all downloads.
	current_uploads:
		Populates info with upload_info for all uploads.
	file_info:
		Returns specific information about file that corresponds to hash (for
		currently running downloads).
		WARNING: Expensive call that includes database access, do not poll this.
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
	share_size:
		Size of all shared files. (bytes)
	search:
		Populates the info vector with results to search.
	set_max_connections:
		Sets connection limit. Disconnects servers at random if over the limit.
	set_download_directory:
		Set location where downloads are to be saved to. Does not change location
		existing downloads are to be saved to.
	set_max_download_rate:
		Sets the upload rate limit.
	start_download:
		Starts a download.
	download_rate:
		Returns average download rate for everything (excluding localhost).
	upload_rate:
		Returns average upload rate for everything (excluding localhost).
	*/
	bool current_download(const std::string & hash, download_status & status);
	void current_downloads(std::vector<download_status> & status);
	void current_uploads(std::vector<upload_status> & info);
	bool file_info(const std::string & hash, std::string & path, boost::uint64_t & tree_size, boost::uint64_t & file_size);
	unsigned get_max_connections();
	unsigned get_max_download_rate();
	unsigned get_max_upload_rate();
	void pause_download(const std::string & hash);
	unsigned prime_count();
	void remove_download(const std::string & hash);
	boost::uint64_t share_size();
	void search(std::string search_word, std::vector<download_info> & Search_Info);
	void set_max_connections(const unsigned & max_connections_in);
	void set_max_download_rate(unsigned download_rate);
	void set_max_upload_rate(unsigned upload_rate);
	void start_download(const download_info & info);
	unsigned download_rate();
	unsigned upload_rate();

private:
	/*
	reconnect_unfinished:
		Resumes downloads upon program start.
	*/
	void reconnect_unfinished();

	database::connection DB;
};
#endif
