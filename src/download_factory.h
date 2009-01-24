#ifndef H_DOWNLOAD_FACTORY
#define H_DOWNLOAD_FACTORY

//custom
#include "client_buffer.h"
#include "database.h"
#include "download.h"
#include "download_connection.h"
#include "download_file.h"
#include "download_hash_tree.h"
#include "download_info.h"
#include "hash_tree.h"

//std
#include <fstream>
#include <list>
#include <typeinfo>

class download_factory
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
