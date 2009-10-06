#include "hash_tree.hpp"

hash_tree::hash_tree(
	const file_info & FI,
	database::pool::proxy DB
):
	hash(FI.hash),
	path(FI.path),
	file_size(FI.file_size),
	row(file_size_to_row(file_size)),
	tree_block_count(row_to_tree_block_count(row)),
	file_hash_offset(row_to_file_hash_offset(row)),
	tree_size(file_size_to_tree_size(file_size)),
	file_block_count(
		file_size % protocol::FILE_BLOCK_SIZE == 0 ?
			file_size / protocol::FILE_BLOCK_SIZE :
			file_size / protocol::FILE_BLOCK_SIZE + 1
	),
	last_file_block_size(
		file_size % protocol::FILE_BLOCK_SIZE == 0 ?
			protocol::FILE_BLOCK_SIZE :
			file_size % protocol::FILE_BLOCK_SIZE
	),
	Block_Request(tree_block_count),
	end_of_good(0)
{
	if(hash != ""){
		boost::shared_ptr<database::table::hash::tree_info>
			TI = database::table::hash::tree_open(hash, DB);
		if(TI){
			//opened existing hash tree
			const_cast<database::blob &>(Blob) = TI->Blob;
		}else{
			//allocate space to reconstruct hash tree
			if(database::table::hash::tree_allocate(hash, tree_size, DB)){
				database::table::hash::set_state(hash, database::table::hash::downloading, DB);
				TI = database::table::hash::tree_open(hash, DB);
				if(TI){
					const_cast<database::blob &>(Blob) = TI->Blob;
				}else{
					throw std::runtime_error("failed to open hash tree");
				}
			}else{
				throw std::runtime_error("failed to allocate blob for hash tree");
			}
		}
	}
}

bool hash_tree::block_info(const boost::uint64_t & block,
	const std::deque<boost::uint64_t> & row, std::pair<boost::uint64_t, unsigned> & info)
{
	boost::uint64_t throw_away;
	return block_info(block, row, info, throw_away);
}

bool hash_tree::block_info(const boost::uint64_t & block,
	const std::deque<boost::uint64_t> & row, std::pair<boost::uint64_t, unsigned> & info,
	boost::uint64_t & parent)
{
	boost::uint64_t offset = 0;          //hash offset from beginning of file to start of row
	boost::uint64_t block_count = 0;     //total block count in all previous rows
	boost::uint64_t row_block_count = 0; //total block count in current row
	for(unsigned x=0; x<row.size(); ++x){
		if(row[x] % protocol::HASH_BLOCK_SIZE == 0){
			row_block_count = row[x] / protocol::HASH_BLOCK_SIZE;
		}else{
			row_block_count = row[x] / protocol::HASH_BLOCK_SIZE + 1;
		}

		//check if block we're looking for is in row
		if(block_count + row_block_count > block){
			info.first = offset + (block - block_count) * protocol::HASH_BLOCK_SIZE;

			//determine size of block
			boost::uint64_t delta = offset + row[x] - info.first;
			if(delta > protocol::HASH_BLOCK_SIZE){
				info.second = protocol::HASH_BLOCK_SIZE;
			}else{
				info.second = delta;
			}
			parent = offset - row[x-1] + block - block_count;

			//convert to bytes
			info.first = info.first * protocol::HASH_SIZE;
			info.second = info.second * protocol::HASH_SIZE;
			parent = parent * protocol::HASH_SIZE;
			return true;
		}
		block_count += row_block_count;
		offset += row[x];
	}
	return false;
}

unsigned hash_tree::block_size(const boost::uint64_t & block_num)
{
	std::pair<boost::uint64_t, unsigned> info;
	if(block_info(block_num, row, info)){
		return info.second;
	}else{
		LOGGER << "programmer error, invalid block specified";
		exit(1);
	}
}

hash_tree::status hash_tree::check_block(const boost::uint64_t & block_num)
{
	SHA1 SHA;
	char buff[protocol::FILE_BLOCK_SIZE];

	if(block_num == 0){
		//special requirements to check root hash, see header documentation for hash
		if(!database::pool::get_proxy()->blob_read(Blob, buff, protocol::HASH_SIZE, 0)){
			return io_error;
		}
		std::memmove(buff + 8, buff, protocol::HASH_SIZE);
		std::memcpy(buff, convert::encode(file_size).data(), 8);
		SHA.init();
		SHA.load(buff, protocol::HASH_SIZE + 8);
		SHA.end();
		if(SHA.hex_hash() == hash){
			return good;
		}else{
			return bad;
		}
	}else{
		std::pair<boost::uint64_t, unsigned> info;
		boost::uint64_t parent;
		if(!block_info(block_num, row, info, parent)){
			//invalid block sent to block_info
			LOGGER << "programmer error\n";
			exit(1);
		}

		//read children
		if(!database::pool::get_proxy()->blob_read(Blob, buff, info.second, info.first)){
			return io_error;
		}

		//create hash for children
		SHA.init();
		SHA.load(buff, info.second);
		SHA.end();

		//verify parent hash is a hash of the children
		if(!database::pool::get_proxy()->blob_read(Blob, buff, protocol::HASH_SIZE, parent)){
			return io_error;
		}
		if(std::memcmp(buff, SHA.raw_hash(), protocol::HASH_SIZE) == 0){
			return good;
		}else{
			return bad;
		}
	}
}

hash_tree::status hash_tree::check()
{
	for(boost::uint32_t x=end_of_good; x<tree_block_count; ++x){
		status Status = check_block(x);
		if(Status == good){
			end_of_good = x+1;
		}else if(Status == bad){
			/*
			We don't store who sent blocks so we can't know which host sent a bad
			block unless the block it sent is equal to end_of_good (see
			documentation in write_block()). We will "re"request this block from
			any host.
			*/
			Block_Request.rerequest_block(x);
			return bad;
		}else if(Status == io_error){
			return io_error;
		}
	}

	if(complete()){
		database::table::hash::set_state(hash, database::table::hash::complete);
	}

	return good;
}

hash_tree::status hash_tree::check_file_block(const boost::uint64_t & file_block_num,
	const char * block, const int & size)
{
	if(!complete()){
		LOGGER << "precondition violated";
		exit(1);
	}

	char parent_buff[protocol::HASH_SIZE];
	if(!database::pool::get_proxy()->blob_read(Blob, parent_buff,
		protocol::HASH_SIZE, file_hash_offset + file_block_num * protocol::HASH_SIZE))
	{
		return io_error;
	}
	SHA1 SHA;
	SHA.init();
	SHA.load(block, size);
	SHA.end();
	if(memcmp(parent_buff, SHA.raw_hash(), protocol::HASH_SIZE) == 0){
		return good;
	}else{
		return bad;
	}
}

bool hash_tree::complete()
{
	return end_of_good == tree_block_count;
}

hash_tree::status hash_tree::create()
{
	//assert the hash tree has not yet been created
	assert(hash == "");

	//open file to generate hash tree for
	std::fstream file(path.c_str(), std::ios::in | std::ios::binary);
	if(!file.good()){
		LOGGER << "error opening file";
		return io_error;
	}

	/*
	Open temp file to write hash tree to. A temp file is used to avoid database
	contention.
	*/
	std::fstream temp(path::hash_tree_temp().c_str(), std::ios::in |
		std::ios::out | std::ios::trunc | std::ios::binary);
	if(!temp.good()){
		LOGGER << "error opening temp file";
		return io_error;
	}

	char buff[protocol::FILE_BLOCK_SIZE];
	SHA1 SHA;

	//do file hashes
	for(boost::uint64_t x=0; x<row.back(); ++x){
		if(boost::this_thread::interruption_requested()){
			return io_error;
		}
		file.read(buff, protocol::FILE_BLOCK_SIZE);
		if(file.gcount() == 0){
			LOGGER << "error reading file";
			return io_error;
		}
		SHA.init();
		SHA.load(buff, file.gcount());
		SHA.end();
		temp.seekp(file_hash_offset + x * protocol::HASH_SIZE, std::ios::beg);
		temp.write(SHA.raw_hash(), protocol::HASH_SIZE);
		if(!temp.good()){
			LOGGER << "error writing temp file";
			return io_error;
		}
	}

	//do all other tree hashes
	for(boost::uint64_t x=tree_block_count - 1; x>0; --x){
		std::pair<boost::uint64_t, unsigned> info;
		boost::uint64_t parent;
		if(!block_info(x, row, info, parent)){
			LOGGER << "programmer error";
			exit(1);
		}
		temp.seekg(info.first, std::ios::beg);
		temp.read(buff, info.second);
		if(temp.gcount() != info.second){
			LOGGER << "error reading temp file";
			return io_error;
		}
		SHA.init();
		SHA.load(buff, info.second);
		SHA.end();
		temp.seekp(parent, std::ios::beg);
		temp.write(SHA.raw_hash(), protocol::HASH_SIZE);
		if(!temp.good()){
			LOGGER << "error writing temp file";
			return io_error;
		}
	}

	//calculate hash
	std::memcpy(buff, convert::encode(file_size).data(), 8);
	std::memcpy(buff + 8, SHA.raw_hash(), protocol::HASH_SIZE);
	SHA.init();
	SHA.load(buff, protocol::HASH_SIZE + 8);
	SHA.end();
	const_cast<std::string &>(hash) = SHA.hex_hash();

	/*
	Copy hash tree in to database. No transaction is used because it this can
	tie up the database for too long when copying a large hash tree. The
	writes are already quite large so a transaction doesn't make much
	performance difference.
	*/

	//check if tree already exists
	if(database::table::hash::tree_open(hash)){
		return good;
	}

	//tree doesn't exist, allocate space for it
	if(!database::table::hash::tree_allocate(hash, tree_size)){
		LOGGER << "error allocating space for hash tree";
		return io_error;
	}

	//copy tree to database using large buffer for performance
	boost::shared_ptr<database::table::hash::tree_info>
		TI = database::table::hash::tree_open(hash);

	//set blob to one we just created
	const_cast<database::blob &>(Blob) = TI->Blob;

	temp.seekg(0, std::ios::beg);
	boost::uint64_t offset = 0, bytes_remaining = tree_size, read_size;
	while(bytes_remaining){
		if(boost::this_thread::interruption_requested()){
			database::table::hash::delete_tree(hash);
			return io_error;
		}
		if(bytes_remaining > protocol::FILE_BLOCK_SIZE){
			read_size = protocol::FILE_BLOCK_SIZE;
		}else{
			read_size = bytes_remaining;
		}
		temp.read(buff, read_size);
		if(temp.gcount() != read_size){
			LOGGER << "error reading temp file";
			database::table::hash::delete_tree(hash);
			return io_error;
		}else{
			if(!database::pool::get_proxy()->blob_write(Blob, buff, read_size, offset)){
				LOGGER << "error doing incremental write to blob";
				database::table::hash::delete_tree(hash);
				return io_error;
			}
			offset += read_size;
			bytes_remaining -= read_size;
		}
	}

	//set tree complete
	end_of_good = tree_block_count;

	return good;
}

boost::uint64_t hash_tree::file_hash_to_tree_hash(boost::uint64_t row_hash,
	std::deque<boost::uint64_t> & row)
{
	boost::uint64_t start_hash = row_hash;
	row.push_front(row_hash);
	if(row_hash == 1){
		return 1;
	}else if(row_hash % protocol::HASH_BLOCK_SIZE == 0){
		row_hash = start_hash / protocol::HASH_BLOCK_SIZE;
	}else{
		row_hash = start_hash / protocol::HASH_BLOCK_SIZE + 1;
	}
	return start_hash + file_hash_to_tree_hash(row_hash, row);
}

boost::uint64_t hash_tree::file_size_to_file_hash(boost::uint64_t file_size)
{
	boost::uint64_t hash_count = file_size / protocol::FILE_BLOCK_SIZE;
	if(file_size % protocol::FILE_BLOCK_SIZE != 0){
		//add one for partial last block
		++hash_count;
	}
	return hash_count;
}

std::deque<boost::uint64_t> hash_tree::file_size_to_row(const boost::uint64_t & file_size)
{
	std::deque<boost::uint64_t> row;
	file_hash_to_tree_hash(file_size_to_file_hash(file_size), row);
	return row;
}

boost::uint64_t hash_tree::file_size_to_tree_size(const boost::uint64_t & file_size)
{
	if(file_size == 0){
		return 0;
	}else{
		std::deque<boost::uint64_t> throw_away;
		return protocol::HASH_SIZE * file_hash_to_tree_hash(file_size_to_file_hash(file_size), throw_away);
	}
}

hash_tree::status hash_tree::read_block(const boost::uint64_t & block_num,
	std::string & block)
{
	char buff[protocol::FILE_BLOCK_SIZE];
	std::pair<boost::uint64_t, unsigned> info;
	if(block_info(block_num, row, info)){
		if(!database::pool::get_proxy()->blob_read(Blob, buff, info.second,
			info.first))
		{
			return io_error;
		}
		block.clear();
		block.assign(buff, info.second);
		return good;
	}else{
		LOGGER << "invalid block number, programming error";
		exit(1);
	}
}

boost::uint64_t hash_tree::row_to_tree_block_count(
	const std::deque<boost::uint64_t> & row)
{
	boost::uint64_t block_count = 0;
	for(int x=0; x<row.size(); ++x){
		if(row[x] % protocol::HASH_BLOCK_SIZE == 0){
			block_count += row[x] / protocol::HASH_BLOCK_SIZE;
		}else{
			block_count += row[x] / protocol::HASH_BLOCK_SIZE + 1;
		}
	}
	return block_count;
}

boost::uint64_t hash_tree::row_to_file_hash_offset(
	const std::deque<boost::uint64_t> & row)
{
	boost::uint64_t file_hash_offset = 0;
	if(row.size() == 0){
		//no hash tree, root hash is file hash
		return file_hash_offset;
	}

	//add up size (bytes) of rows until reaching the last row
	for(int x=0; x<row.size()-1; ++x){
		file_hash_offset += row[x] * protocol::HASH_SIZE;
	}
	return file_hash_offset;
}

hash_tree::status hash_tree::write_block(const int socket_FD,
	const boost::uint64_t & block_num, const std::string & block)
{
	std::pair<boost::uint64_t, unsigned> info;
	if(block_info(block_num, row, info)){
		if(info.second != block.size()){
			LOGGER << "programming error, invalid block size";
			exit(1);
		}
		if(!database::pool::get_proxy()->blob_write(Blob, block.data(),
			block.size(), info.first))
		{
			return io_error;
		}
		status Status = check();
		if(Status == io_error){
			return io_error;
		}else{
			/*
			The hash tree is checked linearly. Cases:
			1. Tree is good < block we just wrote. Block we wrote is bad.
			   end_of_good == block_num. Return bad. The host that sent this bad
				block will be disconnected.
			2. Tree is not good up until block we wrote. end_of_good < block_num.
			   We cannot know if the current block is good or not. Return good.
			*/
			if(end_of_good == block_num){
				//case 1
				return bad;
			}else{
				//case 2
				Block_Request.add_block_local(socket_FD, block_num);
				return good;
			}
		}
	}else{
		LOGGER << "programmer error, invalid block num";
		exit(1);
	}
}
