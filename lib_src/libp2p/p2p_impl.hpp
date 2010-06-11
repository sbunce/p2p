#ifndef H_P2P_IMPL
#define H_P2P_IMPL

//custom
#include "connection_manager.hpp"
#include "db_all.hpp"
#include "hash_tree.hpp"
#include "path.hpp"
#include "prime_generator.hpp"
#include "share_scanner.hpp"
#include "settings.hpp"
#include "share.hpp"

//include
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>
#include <net/net.hpp>
#include <p2p.hpp>
#include <thread_pool.hpp>

//standard
#include <string>
#include <vector>

class p2p_impl : private boost::noncopyable
{
	//instantiate before anything else
	class init
	{
	public:
		init();
	}Init;

public:
	p2p_impl();
	~p2p_impl();

	//documentation for these in p2p.hpp
	unsigned connections();
	unsigned DHT_count();
	unsigned get_max_connections();
	unsigned get_max_download_rate();
	unsigned get_max_upload_rate();
	void remove_download(const std::string & hash);
	void set_max_download_rate(const unsigned rate);
	void set_max_connections(const unsigned connections);
	void set_max_upload_rate(const unsigned rate);
	boost::uint64_t share_size();
	boost::uint64_t share_files();
	void start_download(const p2p::download_info & DI);
	std::list<p2p::transfer_info> transfer();
	boost::optional<p2p::transfer_info> transfer(const std::string & hash);
	unsigned TCP_download_rate();
	unsigned TCP_upload_rate();
	unsigned UDP_download_rate();
	unsigned UDP_upload_rate();

private:
	boost::thread resume_thread;
	connection_manager Connection_Manager;
	share_scanner Share_Scanner;

	/*
	remove_download_thread:
		The remove_download() function schedules a job with Thread_Pool to call
		this function to do removal of a download.
		Note: This is done to make the remove_download() function take less time
			to run.
	resume:
		Thread spawned in this function by ctor to do things needed to resume
		downloads.
	*/
	void remove_download_thread(const std::string hash);
	void resume();
};
#endif
