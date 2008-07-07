#ifndef H_DOWNLOAD_CONNECTION
#define H_DOWNLOAD_CONNECTION

//custom
#include "download.h"

//std
#include <string>

class download; //circular dependency

class download_connection
{
public:
	download_connection(
		download * Download_in,
		const std::string & IP_in
	):
		IP(IP_in),
		socket(-1)
	{
		Download = Download_in;
	}

	//copy ctor
	download_connection(
		const download_connection & DC
	):
		socket(DC.socket),
		IP(DC.IP)
	{
		Download = DC.Download;
	}

	int socket;
	std::string IP;

	/*
	WARNING: this pointer should never be dereferenced. The object this points
	to may be freed at any time.
	*/
	download * Download;
};
#endif
