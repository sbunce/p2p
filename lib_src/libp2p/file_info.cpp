#include "file_info.hpp"

file_info::file_info()
{

}

file_info::file_info(
	const std::string & hash_in,
	const std::string & path_in,
	const boost::uint64_t & file_size_in,
	const std::time_t last_write_time_in
):
	hash(hash_in),
	path(path_in),
	file_size(file_size_in),
	last_write_time(last_write_time_in)
{

}

file_info::file_info(
	const file_info & FI
):
	hash(FI.hash),
	path(FI.path),
	file_size(FI.file_size),
	last_write_time(FI.last_write_time)
{

}
