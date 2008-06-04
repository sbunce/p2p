#ifndef H_DOWNLOAD_PREP
#define H_DOWNLOAD_PREP

//custom
#include "DB_download.h"

//download types
#include "download.h"
#include "download_conn.h"
#include "download_file.h"
#include "download_file_conn.h"
#include "download_hash_tree.h"
#include "download_info.h"

//std
#include <fstream>
#include <list>
#include <typeinfo>

class download_factory
{
public:
	download_factory();

	/*
	start_file - creates a download based on download_info_buffer
	             returns false if download already running, else true and Download set and servers filled
	stop       - stops all downloads, does different actions depending on download type
	             returns false if no download needs to be started in response to the download stopping
	             if true then Download_start will be set and servers filled
	*/
	bool start_file(download_info & info, download *& Download, std::list<download_conn *> & servers);
	bool stop(download * Download_stop, download *& Download_start, std::list<download_conn *> & servers);

private:
	DB_download DB_Download;
};
#endif
