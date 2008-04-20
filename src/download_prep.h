#ifndef H_DOWNLOAD_PREP
#define H_DOWNLOAD_PREP

//std
#include <list>

//download types
#include "download.h"
#include "download_conn.h"
#include "download_file.h"
#include "download_file_conn.h"
#include "download_hash_tree.h"
#include "download_info.h"

//custom
#include "DB_access.h"

class download_prep
{
public:
	download_prep();

	//must be called before any other member function
	void init(volatile int * download_complete_in);

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
	volatile int * download_complete;
	DB_access DB_Access;
};
#endif
