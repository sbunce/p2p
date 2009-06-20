#ifndef H_SHARE_PIPELINE_JOB
#define H_SHARE_PIPELINE_JOB

//boost
#include <boost/cstdint.hpp>

//std
#include <string>

class share_pipeline_job
{
public:
	share_pipeline_job(){}

	share_pipeline_job(
		const std::string & path_in,
		const boost::uint64_t & file_size_in,
		const bool add_in
	):
		path(path_in),
		file_size(file_size_in),
		add(add_in)
	{}

	//set by share_pipeline_0
	bool add; //true if job is to add file
	std::string path;
	boost::uint64_t file_size;

	//set by share_pipeline_1
	std::string hash; //root hash only set if add = true
};
#endif
