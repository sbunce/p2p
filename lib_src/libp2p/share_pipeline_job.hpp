#ifndef H_SHARE_PIPELINE_JOB
#define H_SHARE_PIPELINE_JOB

//custom
#include "share_info.hpp"

//include
#include <boost/cstdint.hpp>

//standard
#include <string>

class share_pipeline_job
{
public:
	share_pipeline_job(){}
	share_pipeline_job(
		const bool add_in,
		const share_info::file & File_in
	):
		add(add_in),
		File(File_in)
	{}

	//true = file to be added, false = file to be removed
	bool add;

	//if File.hash not set then file needs to be hashed
	share_info::file File;
};
#endif
