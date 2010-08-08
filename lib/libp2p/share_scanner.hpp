#ifndef H_SHARE_SCANNER
#define H_SHARE_SCANNER

//custom
#include "connection_manager.hpp"
#include "file_info.hpp"
#include "hash_tree.hpp"
#include "share.hpp"

//include
#include <boost/bind.hpp>
#include <boost/cstdint.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>
#include <logger.hpp>
#include <thread_pool.hpp>

//standard
#include <cassert>
#include <deque>
#include <map>
#include <string>

class share_scanner : private boost::noncopyable
{
	static const unsigned scan_delay_ms = 100;
public:
	share_scanner(connection_manager & Connection_Manager_in);
	~share_scanner();

	/*
	start:
		Start scanning share and hashing files.
	*/
	void start();

private:
	connection_manager & Connection_Manager;

	//used to insure safety of start()
	boost::mutex start_mutex;
	bool started;

	//shared directories
	boost::mutex shared_mutex;
	std::list<std::string> shared;

	/*
	hash_file:
		Threads wait in this function for files to hash.
	remove_missing:
		Removes missing files from share.
	scan:
		Scan share. Schedule files to be hashed.
	start_scan:
		Starts scanning shared directory. This is called by start() and also when
		we catch a filesystem exception.
	*/
	void hash_file(boost::filesystem::recursive_directory_iterator it);
	void remove_missing(share::const_file_iterator it);
	void scan(boost::filesystem::recursive_directory_iterator it);
	void start_scan();

	thread_pool Thread_Pool;
};
#endif
