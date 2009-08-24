#include "file.hpp"

file::file(
	const std::string & hash_in,
	const boost::uint64_t & file_size_in
):
	hash(hash_in),
	file_size(file_size_in),
	Block_Request(file_size % protocol::FILE_BLOCK_SIZE == 0 ?
		file_size / protocol::FILE_BLOCK_SIZE :
		file_size / protocol::FILE_BLOCK_SIZE + 1
	)
{

}
