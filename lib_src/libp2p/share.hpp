//SINGLETON, THREADSAFE, THREAD SPAWNING
#ifndef H_SHARE
#define H_SHARE

//custom
#include "database.hpp"
#include "hash_tree.hpp"
#include "path.hpp"
#include "settings.hpp"
#include "share_pipeline_2_write.hpp"

//include
#include <atomic_int.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/tokenizer.hpp>
#include <boost/utility.hpp>
#include <portable_sleep.hpp>

//standard
#include <ctime>
#include <deque>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>

class share : private boost::noncopyable
{
public:
	share();
	~share();

	/*
	size_bytes:
		Returns size of shared files.
	*/
	boost::uint64_t size_bytes();

private:
	/*
	remove_temporary_files:
		Remove all temporary files used for generating hash trees by the different
		worker threads.
	*/
	void remove_temporary_files();

	share_pipeline_2_write Share_Pipeline_2_Write;
};
#endif

