#ifndef H_P2P
#define H_P2P

//custom
#include "download_info.hpp"
#include "download_status.hpp"
#include "upload_status.hpp"

//include
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>

class p2p_real;

class p2p : private boost::noncopyable
{
public:
	p2p();

	/*
	current_downloads:
		Returns info for all downloads.
	current_uploads:
		Populates info with upload_info for all uploads.
	download_rate:
		Returns average download rate for everything (excluding localhost).
	max_connections:
		Get or set connection limit.
	max_download_rate:
		Get or set download rate limit (bytes).
	max_upload_rate:
		Get or set upload rate limit (bytes).
	pause_download:
		Pause a download.
	prime_count:
		Returns size of prime cache used for key-exchanges.
	remove_download:
		Stops/Removes a running download.
	share_size_bytes:
		Size of all shared files.
	share_size_files:
		The number of files shared.
	start_download:
		Starts a download.
	upload_rate:
		Returns average upload rate for everything (excluding localhost).
	*/
	void current_downloads(std::vector<download_status> & status);
	void current_uploads(std::vector<upload_status> & CU);
	unsigned download_rate();
	unsigned max_connections();
	void max_connections(const unsigned connections);
	unsigned max_download_rate();
	void max_download_rate(const unsigned rate);
	unsigned max_upload_rate();
	void max_upload_rate(const unsigned rate);
	void pause_download(const std::string & hash);
	unsigned prime_count();
	void remove_download(const std::string & hash);
	boost::uint64_t share_size_bytes();
	boost::uint64_t share_size_files();
	void start_download(const download_info & info);
	unsigned upload_rate();

private:
	boost::shared_ptr<p2p_real> P2P_Real;
};
#endif
