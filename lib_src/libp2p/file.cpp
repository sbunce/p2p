#include "file.hpp"

file::file(
	const file_info & FI
):
	path(FI.path),
	file_size(FI.file_size),
	block_count(
		file_size % protocol::FILE_BLOCK_SIZE == 0 ?
			file_size / protocol::FILE_BLOCK_SIZE :
			file_size / protocol::FILE_BLOCK_SIZE + 1
	),
	last_block_size(
		file_size % protocol::FILE_BLOCK_SIZE == 0 ?
			protocol::FILE_BLOCK_SIZE :
			file_size % protocol::FILE_BLOCK_SIZE
	),
	Block_Request(file_size % protocol::FILE_BLOCK_SIZE == 0 ?
		file_size / protocol::FILE_BLOCK_SIZE :
		file_size / protocol::FILE_BLOCK_SIZE + 1
	)
{

}

unsigned file::block_size(const boost::uint64_t block_num)
{
	assert(block_num < block_count);
	if(block_num == block_count - 1){
		//last block
		return last_block_size;
	}else{
		return protocol::FILE_BLOCK_SIZE;
	}
}

bool file::complete()
{
	return false;
}
