//THREADSAFE

#ifndef H_DOWNLOAD_FACTORY
#define H_DOWNLOAD_FACTORY

//boost
#include <boost/utility.hpp>

//custom
#include "client_buffer.hpp"
#include "database.hpp"
#include "download.hpp"
#include "download_connection.hpp"
#include "download_file.hpp"
#include "download_hash_tree.hpp"
#include "download_info.hpp"
#include "hash_tree.hpp"
#include "locking_shared_ptr.hpp"
#include "server_index.hpp"

//std
#include <fstream>
#include <list>
#include <typeinfo>

class download_factory : private boost::noncopyable
{
public:
	download_factory();

	/*
	start - starts download with specified download_info
	        returns empty locking_smart_ptr if could not start
	stop  - stops the download, and possibly starts another one
	        returns empty locking_smart_ptr if no new download needs to be started
	*/
	locking_shared_ptr<download> start(download_info info, std::vector<download_connection> & servers);
	locking_shared_ptr<download> stop(locking_shared_ptr<download> Download_Stop, std::vector<download_connection> & servers);

private:
	locking_shared_ptr<download> start_file(const download_info & info, std::vector<download_connection> & servers);
	locking_shared_ptr<download> start_hash_tree(const download_info & info, std::vector<download_connection> & servers);

	database::connection DB;
	database::table::hash DB_Hash;
	database::table::download DB_Download;
	database::table::search DB_Search;
	database::table::share DB_Share;
};
#endif
