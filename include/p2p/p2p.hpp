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

	void current_downloads(std::vector<download_status> & status);
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
	void start_download(const download_info & info);
	unsigned upload_rate();

private:
	boost::shared_ptr<p2p_real> P2P_Real;
};
#endif
