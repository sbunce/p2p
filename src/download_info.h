#ifndef H_DOWNLOAD_INFO
#define H_DOWNLOAD_INFO

//custom
#include "global.h"

//std
#include <vector>

class download_info
{
public:
	download_info(
		const std::string & hash_in,
		const std::string & name_in,
		const uint64_t & size_in,
		const int speed_in = 0,
		const int percent_complete_in = 0
	):
		hash(hash_in),
		name(name_in),
		size(size_in),
		speed(speed_in),
		percent_complete(percent_complete_in),
		resumed(false)
	{}

	std::string hash;        //root hash of hash tree
	std::string name;        //name of the file
	uint64_t size;           //size of the file (bytes)

	//information unique to the server
	std::vector<std::string> IP;

	//these are set when the download is running
	int percent_complete;
	int speed;

	/*
	This is only relevant when download_info is used to start a download.

	Resumed downloads must set this to true. There is a check which exists when
	starting a download that won't let downloads be added twice. This check is
	dependent on looking at whether or not the download information already exists
	in the database. This overrides that check. When resuming a download the
	download information is in the database but no download is actually started.
	*/
	bool resumed;

	void print_info()
	{
		std::cout << "hash:             " << hash << "\n";
		std::cout << "name:             " << name << "\n";
		std::cout << "size:             " << size << "\n";
		std::cout << "percent_complete: " << percent_complete << "\n";
		std::cout << "speed:            " << speed << "\n";
	}
};
#endif
