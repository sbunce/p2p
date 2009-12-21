#include "file.hpp"

file::file(
	const file_info & FI
):
	path(FI.path),
	file_size(FI.file_size),
	block_count(
		file_size % protocol::file_block_size == 0 ?
			file_size / protocol::file_block_size :
			file_size / protocol::file_block_size + 1
	),
	last_block_size(
		file_size % protocol::file_block_size == 0 ?
			protocol::file_block_size :
			file_size % protocol::file_block_size
	),
	Block_Request(file_size % protocol::file_block_size == 0 ?
		file_size / protocol::file_block_size :
		file_size / protocol::file_block_size + 1
	)
{
	boost::shared_ptr<database::table::share::info>
		info = database::table::share::find(FI.hash);
	if(info){
		if(info->file_state == database::table::share::complete){
//DEBUG, set Block_Request to complete
			//Block_Request
		}
	}else{
		LOGGER << "could not locate file info in share";
	}
}

unsigned file::block_size(const boost::uint64_t block_num)
{
	assert(block_num < block_count);
	if(block_num == block_count - 1){
		//last block
		return last_block_size;
	}else{
		return protocol::file_block_size;
	}
}

bool file::complete()
{
//DEBUG, always return true for testing
	return true;
}
