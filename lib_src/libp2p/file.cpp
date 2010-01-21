#include "file.hpp"

file::file(
	const file_info & FI
):
	path(FI.path),
	file_size(FI.file_size),
	file_block_count(
		file_size % protocol::file_block_size == 0 ?
			file_size / protocol::file_block_size :
			file_size / protocol::file_block_size + 1
	),
	last_block_size(
		file_size % protocol::file_block_size == 0 ?
			protocol::file_block_size :
			file_size % protocol::file_block_size
	)
{

}

unsigned file::block_size(const boost::uint64_t block_num)
{
	assert(block_num < file_block_count);
	if(block_num == file_block_count - 1){
		return last_block_size;
	}else{
		return protocol::file_block_size;
	}
}

boost::uint64_t file::calc_file_block_count(boost::uint64_t file_size)
{
	if(file_size % protocol::file_block_size == 0){
		return file_size / protocol::file_block_size;
	}else{
		return file_size / protocol::file_block_size + 1;
	}		
}

bool file::read_block(const boost::uint64_t block_num, network::buffer & buf)
{
	std::fstream fin(path.c_str(), std::ios::in | std::ios::out | std::ios::binary);
	fin.seekg(block_num * protocol::file_block_size);
	if(fin.is_open()){
		unsigned size = block_size(block_num);
		buf.tail_reserve(size);
		fin.read(reinterpret_cast<char *>(buf.tail_start()), size);
		if(fin.gcount() == size){
			buf.tail_resize(size);
			return true;
		}else{
			buf.tail_reserve(0);
			return false;
		}
	}else{
		LOGGER << "failed to open file";
		return false;
	}
}

bool file::write_block(const boost::uint64_t block_num, const network::buffer & buf)
{
	std::fstream fout(path.c_str(), std::ios::in | std::ios::out | std::ios::binary);
	if(!fout.is_open()){
		LOGGER << "failed to open file";
		return false;
	}
	fout.seekp(block_num * protocol::file_block_size);
	fout.write(reinterpret_cast<char *>(const_cast<network::buffer &>(buf).data()),
		buf.size());
	if(!fout.is_open()){
		LOGGER << "failed to open file";
		return false;
	}
	return true;
}
