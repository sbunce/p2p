//THREADSAFE, CTOR THREAD SPAWNING
#ifndef H_SHARE_SCAN
#define H_SHARE_SCAN

//custom
#include "database.hpp"
#include "hash_tree.hpp"
#include "path.hpp"
#include "settings.hpp"
#include "shared_files.hpp"
#include "share_scan_2_write.hpp"

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

class share_scan : private boost::noncopyable
{
public:
	share_scan(shared_files & Shared_Files_in);
	~share_scan();

private:
	//last stage of pipeline
	share_scan_2_write Share_Scan_2_Write;
};
#endif

