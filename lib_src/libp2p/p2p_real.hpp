//THREADSAFE, CTOR THREAD SPAWNING
#ifndef H_P2P_REAL
#define H_P2P_REAL

//custom
#include "database.hpp"
#include "hash_tree.hpp"
#include "path.hpp"
#include "prime_generator.hpp"
#include "share.hpp"
#include "settings.hpp"
#include "thread_pool.hpp"

//include
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>
//#include <network/network.hpp>
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

	//documentation for these in p2p.hpp
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
	boost::uint64_t share_size_bytes();
	boost::uint64_t share_size_files();
	void start_download(const download_info & DI);
	unsigned upload_rate();

private:
	/*
	The values that database::table::get_<"max_connections" | "max_download_rate"
	| "max_upload_rate"> return are proxied here so that the database calls to
	get or set the values in the database can be done asynchronously. This is
	important to make these calls very responsive.
	*/
	atomic_int<unsigned> max_connections_proxy;
	atomic_int<unsigned> max_download_rate_proxy;
	atomic_int<unsigned> max_upload_rate_proxy;

	prime_generator Prime_Generator;
	share Share;

	/*
	We make a shared_ptr to the proactor so we can make sure the class is
	instantiated before we register call backs.
	*/
	//network::proactor Proactor;
};
#endif
