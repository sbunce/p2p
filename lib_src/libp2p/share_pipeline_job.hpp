#ifndef H_SHARE_PIPELINE_JOB
#define H_SHARE_PIPELINE_JOB

//include
#include <boost/cstdint.hpp>

//standard
#include <string>

class share_pipeline_job
{
public:
	enum type {
		HASH_FILE,         //file needs to be hashed
		REMOVE_FILE        //file removed
	} Type;

	share_pipeline_job(){}

	share_pipeline_job(
		const std::string & path_in,
		const boost::uint64_t & file_size_in,
		const type Type_in
	):
		path(path_in),
		file_size(file_size_in),
		Type(Type_in)
	{}

	//set by share_pipeline_0
	std::string path;
	boost::uint64_t file_size;

	//set by share_pipeline_1 if Job_Type = HASH_FILE
	std::string hash;
};
#endif
