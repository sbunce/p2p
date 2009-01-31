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
	        returns true if download could start, else false if error
	stop  - stops the download, and possibly starts another one
	        if return true then Download_Start is set and the servers list is filled
	*/
	bool start(download_info info, download *& Download, std::list<download_connection> & servers);
	bool stop(download * Download_Stop, download *& Download_Start, std::list<download_connection> & servers);

private:
	download * start_file(const download_info & info, std::list<download_connection> & servers);
	download * start_hash_tree(const download_info & info, std::list<download_connection> & servers);

	database::connection DB;
	database::table::hash DB_Hash;
	database::table::download DB_Download;
	database::table::search DB_Search;
	database::table::share DB_Share;
};
#endif
