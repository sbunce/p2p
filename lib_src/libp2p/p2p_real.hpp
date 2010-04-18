#ifndef H_P2P_REAL
#define H_P2P_REAL

//custom
#include "connection_manager.hpp"
#include "db_all.hpp"
#include "hash_tree.hpp"
#include "kademlia.hpp"
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
#include <network/network.hpp>
#include <p2p.hpp>
#include <thread_pool.hpp>

//standard
#include <string>
#include <vector>

class p2p_real : private boost::noncopyable
{
	//instantiate before anything else
	class init
	{
	public:
		init()
		{
			db::init::create_all();
		}
	}Init;

public:
	p2p_real();
	~p2p_real();

	//documentation for these in p2p.hpp
	unsigned download_rate();
	unsigned max_connections();
	void max_connections(unsigned connections);
	unsigned max_download_rate();
	void max_download_rate(const unsigned rate);
	unsigned max_upload_rate();
	void max_upload_rate(const unsigned rate);
	void remove_download(const std::string & hash);
	boost::uint64_t share_size_bytes();
	boost::uint64_t share_size_files();
	void start_download(const p2p::download & D);
	void transfers(std::vector<p2p::transfer> & T);
	unsigned upload_rate();

private:
	//thread spawned by ctor to do actions on startup
	boost::thread resume_thread;

	connection_manager Connection_Manager;
	kademlia Kademlia;
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
