#include "file.hpp"

file::file(
	const shared_files::file & File
):
	hash(File.hash),
	file_size(File.file_size),
	Block_Request(file_size % protocol::FILE_BLOCK_SIZE == 0 ?
		file_size / protocol::FILE_BLOCK_SIZE :
		file_size / protocol::FILE_BLOCK_SIZE + 1
	)
{

}
