//THREADSAFE, CTOR THREAD SPAWNING
#ifndef H_P2P_REAL
#define H_P2P_REAL

//custom
#include "connection.hpp"
#include "database.hpp"
#include "hash_tree.hpp"
#include "path.hpp"
#include "prime_generator.hpp"
#include "share.hpp"
#include "settings.hpp"

//include
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>
#include <network/network.hpp>
#include <p2p/p2p.hpp>
#include <thread_pool.hpp>

//standard
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

class p2p_real : private boost::noncopyable
{
	//sets up all directories and database before anything else is instantiated
	class init
	{
	public:
		init()
		{
			path::create_required_directories();
			database::init::create_all();
		}
	}Init;

public:
	p2p_real();
	~p2p_real();

	//documentation for these in p2p.hpp
	unsigned download_rate();
	void downloads(std::vector<download_status> & CD);
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
	void start_download(const download_info & DI);
	unsigned upload_rate();
	void uploads(std::vector<upload_status> & CU);

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

	//indexes share
	share Share;

	/*
	Connection state will be stored in the connection object. Also call backs
	will be registered in connect_call_back to do call backs directly to the
	connection objects stored in this container. The mutex locks access to this
	container because it's possible there will be multiple threads doing call
	backs to connect_call_back and disconnect_call_back concurrently which both
	modify the Connection container.
	*/
	boost::mutex Connection_mutex;
	std::map<int, boost::shared_ptr<connection> > Connection;

	//used to schedule long jobs so the GUI doesn't have to wait
	thread_pool Thread_Pool;

	/*
	Proactor which does call backs for network activity.
	Note: This is specified last in header because it starts threads in it's ctor
		which use objects in p2p_real. We specify it last because we want to make
		sure the objects it uses are constructed before it uses them.
	*/
	network::proactor Proactor;

	/*
	connect_call_back:
		Proactor calls back to this when a socket connects.
	disconnect_call_back:
		Proactor calls back to this when a socket normally disconnects.
	failed_connect_call_back:
		Proactor calls back to this when a connection fails.
	*/
	void connect_call_back(network::sock & S);
	void disconnect_call_back(network::sock & S);
	void failed_connect_call_back(network::sock & S);
};
#endif
