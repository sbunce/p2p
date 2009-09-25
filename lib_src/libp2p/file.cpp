#include "file.hpp"

file::file(
	const file_info & FI
):
	hash(FI.hash),
	path(FI.path),
	file_size(FI.file_size)

/*
	Block_Request(file_size % protocol::FILE_BLOCK_SIZE == 0 ?
		file_size / protocol::FILE_BLOCK_SIZE :
		file_size / protocol::FILE_BLOCK_SIZE + 1
	)
*/
{

}
