#ifndef H_SHARE_SCAN_JOB
#define H_SHARE_SCAN_JOB

//custom
#include "file_info.hpp"

//include
#include <boost/cstdint.hpp>

//standard
#include <string>

class share_scan_job
{
public:
	share_scan_job();
	share_scan_job(
		const bool add_in,
		const file_info & FI_in
	);

	//true = file to be added, false = file to be removed
	bool add;

	//if File.hash not set then file needs to be hashed
	file_info FI;
};
#endif
