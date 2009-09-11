//SINGLETON, THREADSAFE, THREAD SPAWNING
#ifndef H_SHARE
#define H_SHARE

//custom
#include "database.hpp"
#include "hash_tree.hpp"
#include "path.hpp"
#include "settings.hpp"
#include "share_info.hpp"
#include "share_pipeline_2_write.hpp"

//include
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/tokenizer.hpp>
#include <boost/utility.hpp>

//standard
#include <ctime>
#include <iostream>
#include <map>
#include <string>

class share : private boost::noncopyable
{
public:
	share();
	~share();

	/*
	bytes:
		Returns total size of shared files.
	files:
		Returns total number of shared files.
	*/
	boost::uint64_t bytes();
	boost::uint64_t files();

private:
	//contains files in share
	share_info Share_Info;

	//last stage of pipeline
	share_pipeline_2_write Share_Pipeline_2_Write;
};
#endif

