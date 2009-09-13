#ifndef H_SHARE_SCAN_JOB
#define H_SHARE_SCAN_JOB

//custom
#include "shared_files.hpp"

//include
#include <boost/cstdint.hpp>

//standard
#include <string>

class share_scan_job
{
public:
	share_scan_job(){}
	share_scan_job(
		const bool add_in,
		const shared_files::file & File_in
	):
		add(add_in),
		File(File_in)
	{}

	//true = file to be added, false = file to be removed
	bool add;

	//if File.hash not set then file needs to be hashed
	shared_files::file File;
};
#endif
