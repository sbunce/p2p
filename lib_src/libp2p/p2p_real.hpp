//THREADSAFE, CTOR THREAD SPAWNING
#ifndef H_P2P_REAL
#define H_P2P_REAL

//custom
#include "connection_manager.hpp"
#include "database.hpp"
#include "hash_tree.hpp"
#include "path.hpp"
#include "prime_generator.hpp"
#include "share_scan.hpp"
#include "settings.hpp"
#include "share.hpp"

//include
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>
//#include <network/network.hpp>
#include <p2p.hpp>
#include <thread_pool.hpp>

//standard
#include <string>
#include <vector>

class p2p_real : private boost::noncopyable
{
	//set up all directories and database before anything else
	database::init init;
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
	void pause_download(const std::string & hash);
	unsigned prime_count();
	void remove_download(const std::string & hash);
	boost::uint64_t share_size_bytes();
	boost::uint64_t share_size_files();
	void start_download(const p2p::download & D);
	unsigned supported_connections();
	void transfers(std::vector<p2p::transfer> & T);
	unsigned upload_rate();

private:
	//thread spawned by ctor to do actions on startup
	boost::thread resume_thread;

	/*
	The values to store in the database are stored here and a job is scheduled
	with Thread_Pool to do the actual database read/write. This allows the
	functions which get/set these to return immediately instead of having to wait
	for database access.
	*/
	atomic_int<unsigned> max_connections_proxy;
	atomic_int<unsigned> max_download_rate_proxy;
	atomic_int<unsigned> max_upload_rate_proxy;

	/*
	Shared_Files:
		Container which holds information for all shared files.
	Share_Scan:
		Scans shared directories looking for new and modified files.
	*/
	share Share;
	share_scan Share_Scan;

	//used to schedule long jobs so the GUI doesn't have to wait
	thread_pool Thread_Pool;

	/*
	Proactor does call backs for network activity. The call backs are done to the
	Connection_Manager.
	Note: Proactor is specified last in header because it starts threads in it's
		ctor which use objects in p2p_real. We specify it last because we want to
		make sure the objects it uses are constructed before it uses them.
	*/
	network::proactor Proactor;
	connection_manager Connection_Manager;

	/*
	resume:
		Thread spawned in this function by ctor to do things needed to resume
		downloads.
	*/
	void resume();
};
#endif
