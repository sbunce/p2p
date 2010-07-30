#include "tree_info.hpp"

tree_info::tree_info(const file_info & FI):
	hash(FI.hash),
	file_size(FI.file_size),
	row(file_size_to_row(file_size)),
	tree_block_count(row_to_tree_block_count(row)),
	file_hash_offset(row_to_file_hash_offset(row)),
	tree_size(file_size_to_tree_size(file_size)),
	file_block_count(
		file_size % protocol_tcp::file_block_size == 0 ?
			file_size / protocol_tcp::file_block_size :
			file_size / protocol_tcp::file_block_size + 1
	),
	last_file_block_size(
		file_size % protocol_tcp::file_block_size == 0 ?
			protocol_tcp::file_block_size :
			file_size % protocol_tcp::file_block_size
	)
{

}

bool tree_info::block_info(const boost::uint64_t block,
	std::pair<boost::uint64_t, unsigned> & info) const
{
	boost::uint64_t throw_away;
	return block_info(block, info, throw_away);
}

bool tree_info::block_info(const boost::uint64_t block,
	std::pair<boost::uint64_t, unsigned> & info, boost::uint64_t & parent) const
{
	/*
	For simplicity this function operates on RRNs until just before it is done
	when it converts to bytes.
	*/
	boost::uint64_t offset = 0;          //hash offset from beginning of file to start of row
	boost::uint64_t block_count = 0;     //total block count in all previous rows
	boost::uint64_t row_block_count = 0; //total block count in current row
	for(unsigned x=0; x<row.size(); ++x){
		if(row[x] % protocol_tcp::hash_block_size == 0){
			row_block_count = row[x] / protocol_tcp::hash_block_size;
		}else{
			row_block_count = row[x] / protocol_tcp::hash_block_size + 1;
		}

		//check if block we're looking for is in row
		if(block_count + row_block_count > block){
			info.first = offset + (block - block_count) * protocol_tcp::hash_block_size;

			//determine size of block
			boost::uint64_t delta = offset + row[x] - info.first;
			if(delta > protocol_tcp::hash_block_size){
				info.second = protocol_tcp::hash_block_size;
			}else{
				info.second = delta;
			}

			if(x != 0){
				//if not the root hash there is a parent
				parent = offset - row[x-1] + block - block_count;
			}

			//convert to bytes
			parent = parent * SHA1::bin_size;
			info.first = info.first * SHA1::bin_size;
			info.second = info.second * SHA1::bin_size;
			return true;
		}
		block_count += row_block_count;
		offset += row[x];
	}
	return false;
}

unsigned tree_info::block_size(const boost::uint64_t block_num) const
{
	std::pair<boost::uint64_t, unsigned> info;
	if(block_info(block_num, info)){
		return info.second;
	}else{
		LOG << "invalid block";
		exit(1);
	}
}

boost::uint64_t tree_info::calc_tree_block_count(boost::uint64_t file_size)
{
	std::deque<boost::uint64_t> row = file_size_to_row(file_size);
	return row_to_tree_block_count(row);
}

std::pair<std::pair<boost::uint64_t, boost::uint64_t>, bool>
	tree_info::file_block_children(const boost::uint64_t block) const
{
	std::pair<boost::uint64_t, unsigned> info;
	if(block_info(block, info)){
		std::pair<std::pair<boost::uint64_t, boost::uint64_t>, bool> pair;
		if(info.first >= file_hash_offset){
			pair.first.first = (info.first - file_hash_offset) / SHA1::bin_size;
			pair.first.second = ((info.first + info.second) - file_hash_offset) / SHA1::bin_size;
			pair.second = true;
			return pair;
		}else{
			pair.second = false;
			return pair;
		}
	}else{
		LOG << "invalid block";
		exit(1);
	}
}

boost::uint64_t tree_info::file_hash_to_tree_hash(boost::uint64_t row_hash,
	std::deque<boost::uint64_t> & row)
{
	boost::uint64_t start_hash = row_hash;
	row.push_front(row_hash);
	if(row_hash == 1){
		return 1;
	}else if(row_hash % protocol_tcp::hash_block_size == 0){
		row_hash = start_hash / protocol_tcp::hash_block_size;
	}else{
		row_hash = start_hash / protocol_tcp::hash_block_size + 1;
	}
	return start_hash + file_hash_to_tree_hash(row_hash, row);
}

boost::uint64_t tree_info::file_size_to_file_hash(const boost::uint64_t file_size)
{
	boost::uint64_t hash_count = file_size / protocol_tcp::file_block_size;
	if(file_size % protocol_tcp::file_block_size != 0){
		//add one for partial last block
		++hash_count;
	}
	return hash_count;
}

std::deque<boost::uint64_t> tree_info::file_size_to_row(const boost::uint64_t file_size)
{
	std::deque<boost::uint64_t> row;
	file_hash_to_tree_hash(file_size_to_file_hash(file_size), row);
	return row;
}

boost::uint64_t tree_info::file_size_to_tree_size(const boost::uint64_t file_size)
{
	if(file_size == 0){
		return 0;
	}else{
		std::deque<boost::uint64_t> throw_away;
		return SHA1::bin_size * file_hash_to_tree_hash(file_size_to_file_hash(file_size), throw_away);
	}
}

boost::uint64_t tree_info::row_to_file_hash_offset(const std::deque<boost::uint64_t> & row)
{
	boost::uint64_t file_hash_offset = 0;
	if(row.size() == 0){
		//no hash tree, root hash is file hash
		return file_hash_offset;
	}
	//add up size (bytes) of rows until reaching the last row
	for(int x=0; x<row.size()-1; ++x){
		file_hash_offset += row[x] * SHA1::bin_size;
	}
	return file_hash_offset;
}

boost::uint64_t tree_info::row_to_tree_block_count(
	const std::deque<boost::uint64_t> & row)
{
	boost::uint64_t block_count = 0;
	for(int x=0; x<row.size(); ++x){
		if(row[x] % protocol_tcp::hash_block_size == 0){
			block_count += row[x] / protocol_tcp::hash_block_size;
		}else{
			block_count += row[x] / protocol_tcp::hash_block_size + 1;
		}
	}
	return block_count;
}

std::pair<std::pair<boost::uint64_t, boost::uint64_t>, bool>
	tree_info::tree_block_children(const boost::uint64_t block) const
{
	boost::uint64_t offset = 0;          //hash offset from beginning of file to start of row
	boost::uint64_t block_count = 0;     //total block count in all previous rows
	boost::uint64_t row_block_count = 0; //total block count in current row
	for(unsigned x=0; x<row.size(); ++x){
		if(row[x] % protocol_tcp::hash_block_size == 0){
			row_block_count = row[x] / protocol_tcp::hash_block_size;
		}else{
			row_block_count = row[x] / protocol_tcp::hash_block_size + 1;
		}
		//check if block we're looking for is in row
		if(block_count + row_block_count > block){
			//offset to start of block (RRN, hash offset)
			boost::uint64_t block_offset = offset + (block - block_count)
				* protocol_tcp::hash_block_size;

			if(block_offset * SHA1::bin_size >= file_hash_offset){
				//leaf, no hash block children
				std::pair<std::pair<boost::uint64_t, boost::uint64_t>, bool> pair;
				pair.second = false;
				return pair;
			}

			//determine size of block
			boost::uint64_t delta = offset + row[x] - block_offset;
			unsigned block_size;
			if(delta > protocol_tcp::hash_block_size){
				block_size = protocol_tcp::hash_block_size;
			}else{
				block_size = delta;
			}

			std::pair<std::pair<boost::uint64_t, boost::uint64_t>, bool> pair;
			pair.second = true;
			pair.first.first =
				(block_offset - offset) +        //parents in current row before block
				(block_count + row_block_count); //block number of start of next row
			pair.first.second = pair.first.first + block_size;
			return pair;
		}
		block_count += row_block_count;
		offset += row[x];
	}
	LOG << "invalid block";
	exit(1);
}
