#ifndef H_DOWNLOAD_PREP
#define H_DOWNLOAD_PREP

//std
#include <vector>

//download types
#include "download.h"
#include "download_conn.h"
#include "download_file.h"
#include "download_file_conn.h"
#include "download_hash_tree.h"

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
	             returns NULL pointer if no download could be created
	stop       - stops all downloads, does different actions depending on download type
	*/
	download * start_file(DB_access::download_info_buffer & info, std::vector<download_conn *> & servers);
	download * stop(download * Download);

private:
	volatile int * download_complete;
	DB_access DB_Access;
};
#endif
