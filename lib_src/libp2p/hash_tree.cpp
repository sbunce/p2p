#include "hash_tree.hpp"

hash_tree::hash_tree(
	const file_info & FI
):
	hash(FI.hash),
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
	set_state_downloading(true),
	block_status(tree_block_count),
	end_of_good(0)
{
	if(tree_block_count != 0){
		boost::shared_ptr<database::table::hash::tree_info>
			TI = database::table::hash::tree_open(hash);
		if(TI){
			Blob = TI->Blob;
		}else{
			throw std::exception();
		}
	}
}

bool hash_tree::block_info(const boost::uint64_t & block, const std::deque<boost::uint64_t> & row,
	std::pair<boost::uint64_t, unsigned> & info)
{
	boost::uint64_t throw_away;
	return block_info(block, row, info, throw_away);
}

bool hash_tree::block_info(const boost::uint64_t & block, const std::deque<boost::uint64_t> & row,
	std::pair<boost::uint64_t, unsigned> & info, boost::uint64_t & parent)
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

		if(block_count + row_block_count > block){
			//end of row greater than block we're looking for, block exists in row
			info.first = offset + (block - block_count) * protocol::HASH_BLOCK_SIZE;

			//hashes between offset and end of current row
			boost::uint64_t delta = offset + row[x] - info.first;

			//determine size of block
			if(delta > protocol::HASH_BLOCK_SIZE){
				//full hash block
				info.second = protocol::HASH_BLOCK_SIZE;
			}else{
				//partial hash block
				info.second = delta;
			}

			//locate parent
			if(x != 0){
				//only set parent if not on first row (parent of first row is root hash)
				parent = offset - row[x-1] //start of previous row
					+ block - block_count;  //hash offset in to previous row is block offset in to current row
			}

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

void hash_tree::check()
{
	for(boost::uint32_t x=end_of_good; x<tree_block_count; ++x){
		status Status = check_block(x);
		if(Status == good){
			Block_Request.add_block(x);
			end_of_good = x+1;
		}else if(Status == bad){
			Block_Request.remove_block(x);
			block_status[x] = 0;
			LOGGER << "bad block " << x << " in tree " << hash;
			return;
		}else if(Status == io_error){
			return;
		}
	}

	if(complete()){
		database::table::hash::set_state(hash, database::table::hash::complete);
	}
}

hash_tree::status hash_tree::check_block(const boost::uint64_t & block_num)
{
	if(tree_block_count == 0){
		//only one hash, it's the root hash and the file hash
		return good;
	}

	std::pair<boost::uint64_t, unsigned> info;
	boost::uint64_t parent;
	if(!block_info(block_num, row, info, parent)){
		//invalid block sent to block_info
		LOGGER << "programmer error\n";
		exit(1);
	}

	char block_buff[protocol::FILE_BLOCK_SIZE];
	if(!database::pool::get_proxy()->blob_read(Blob, block_buff,
		info.second, info.first))
	{
		return io_error;
	}

	//create hash for children
	SHA1 SHA;
	SHA.init();
	SHA.load(block_buff, info.second);
	SHA.end();

	//check child hash
	if(block_num == 0){
		//first row has to be checked against root hash
		std::string hash_bin = convert::hex_to_bin(hash);
		if(hash_bin.empty()){
			LOGGER << "invalid hex";
			exit(1);
		}
		if(strncmp(hash_bin.data(), SHA.raw_hash(), protocol::HASH_SIZE) == 0){
			return good;
		}else{
			return bad;
		}
	}else{
		if(!database::pool::get_proxy()->blob_read(Blob, block_buff,
			protocol::HASH_SIZE, parent))
		{
			return io_error;
		}

		if(strncmp(block_buff, SHA.raw_hash(), protocol::HASH_SIZE) == 0){
			return good;
		}else{
			return bad;
		}
	}
}

hash_tree::status hash_tree::check_incremental(boost::uint32_t & bad_block)
{
	for(boost::uint32_t x=end_of_good; x<tree_block_count && block_status[x]; ++x){
		status Status = check_block(x);
		if(Status == good){
			Block_Request.add_block(x);
			end_of_good = x+1;
		}else if(Status == bad){
			Block_Request.remove_block(x);
			block_status[x] = 0;
			bad_block = x;
			if(bad_block != 0){
				LOGGER << "bad block " << x << " in tree " << hash;
			}
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

hash_tree::status hash_tree::check_file_block(const boost::uint64_t & file_block_num, const char * block, const int & size)
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
	if(strncmp(parent_buff, SHA.raw_hash(), protocol::HASH_SIZE) == 0){
		return good;
	}else{
		return bad;
	}
}

bool hash_tree::complete()
{
	return end_of_good == tree_block_count;
}

hash_tree::status hash_tree::create(file_info & FI)
{
	std::fstream fin(FI.path.c_str(), std::ios::in | std::ios::binary);
	if(!fin.good()){
		return io_error;
	}

	/*
	Scratch files for building the tree.

	The "upside_down" file contains the tree as it's initially built, with the
	file hashes at the beginning of the file. This is not convenient for later
	use because the file would need to be read backwards. Because of this the
	tree is reversed, copied from the upside_down file to the rightside_up
	file in such a way that the second row is at the start of the file.

	Note: The root hash is not included in the file because that would be
	      redundant information and a waste of space.
	*/
	std::fstream upside_down(path::upside_down().c_str(), std::ios::in |
		std::ios::out | std::ios::trunc | std::ios::binary);
	if(!upside_down.good()){
		LOGGER << "error opening upside_down file";
		return io_error;
	}
	std::fstream rightside_up(path::rightside_up().c_str(), std::ios::in |
		std::ios::out | std::ios::trunc | std::ios::binary);
	if(!rightside_up.good()){
		LOGGER << "error opening rightside_up file";
		return io_error;
	}

	char block_buff[protocol::FILE_BLOCK_SIZE];
	SHA1 SHA;

	//create all file block hashes
	boost::uint64_t blocks_read = 0;
	boost::uint64_t file_size = 0;
	while(true){
		if(boost::this_thread::interruption_requested()){
			return io_error;
		}

		fin.read(block_buff, protocol::FILE_BLOCK_SIZE);
		if(fin.gcount() == 0){
			break;
		}

		SHA.init();
		SHA.load(block_buff, fin.gcount());
		SHA.end();
		upside_down.write(SHA.raw_hash(), protocol::HASH_SIZE);
		if(!upside_down.good()){
			LOGGER << "error writing to upside_down file";
			return io_error;
		}

		++blocks_read;
		file_size += fin.gcount();
		if(file_size > FI.file_size){
			return io_error;
		}
	}

	if(fin.bad() || !fin.eof()){
		LOGGER << "error reading file \"" << FI.path << "\"";
		return io_error;
	}

	if(blocks_read == 0){
		//do not generate hash trees for empty files
		return good;
	}

	//base case, the file size is == one block
	if(blocks_read == 1){
		FI.hash = SHA.hex_hash();
		return good;
	}

	//hack for windows
	#ifdef _WIN32
		#undef max
	#endif
	boost::uint64_t tree_size = file_size_to_tree_size(file_size);
	if(tree_size > std::numeric_limits<int>::max()){
		LOGGER << "file at location \"" << FI.path << "\" would generate hash tree beyond max SQLite3 blob size";
		return bad;
	}

	create_recurse(upside_down, rightside_up, 0, blocks_read, FI.hash, block_buff, SHA);

	if(FI.hash.empty()){
		//tree didn't generate
		return io_error;
	}else{
		/*
		Tree sucessfully created. Look in the database to see if the tree already
		exists in the hash table. If it does there's no reason to replace it with
		the tree just generated.

		Note: The state of the hash tree is only set to reserved after this
		finishes. If the state is not set to complete then the tree will be
		deleted on next program start. (think garbage collection)
		*/
		assert(tree_size == rightside_up.tellp());

		/*
		Note: There is no transaction for this section. It would be detrimental to
		have a transaction here. The database would be tied up for too long while
		copying a very large hash tree. Also, performance is not improved by using
		a transaction because the blob writes are already quite large.

		It is possible that the hash tree could be deleted while we are copying to
		it. However, if this happens we return an error and the scanner tries the
		file again later.
		*/
		database::pool::proxy DB;
		if(database::table::hash::tree_open(FI.hash, DB)){
			//hash tree already exists
			return good;
		}

		//tree doesn't exist, allocate space for it
		if(!database::table::hash::tree_allocate(FI.hash, tree_size, DB)){
			//could not allocate blob for hash tree
			return io_error;
		}

		/*
		Copy tree from temp file to database. Using a transaction here would be
		problematic for very large hash trees. Because we're not using a
		transaction it is possible that two threads write to a hash tree at the
		same time. However, this is ok because they'd both be writing the same
		data.
		*/
		boost::shared_ptr<database::table::hash::tree_info>
			TI = database::table::hash::tree_open(FI.hash, DB);
		rightside_up.seekg(0, std::ios::beg);
		int offset = 0, bytes_remaining = tree_size, read_size;
		while(bytes_remaining){
			if(boost::this_thread::interruption_requested()){
				database::table::hash::delete_tree(FI.hash, DB);
				return io_error;
			}
			if(bytes_remaining > protocol::FILE_BLOCK_SIZE){
				read_size = protocol::FILE_BLOCK_SIZE;
			}else{
				read_size = bytes_remaining;
			}
			rightside_up.read(block_buff, read_size);
			if(rightside_up.gcount() != read_size){
				LOGGER << "error reading rightside_up file";
				database::table::hash::delete_tree(FI.hash, DB);
				return io_error;
			}else{
				if(!DB->blob_write(TI->Blob, block_buff, read_size, offset)){
					LOGGER << "error doing incremental write to blob";
					database::table::hash::delete_tree(FI.hash, DB);
					return io_error;
				}
				offset += read_size;
				bytes_remaining -= read_size;
			}
		}
		return good;
	}
}

bool hash_tree::create_recurse(std::fstream & upside_down, std::fstream & rightside_up,
	boost::uint64_t start_RRN, boost::uint64_t end_RRN, std::string & hash,
	char * block_buff, SHA1 & SHA)
{
	/*
	A failure of this function is indicated when hash is not set after it
	returns. If hash is not set then there was an io_error.
	*/

	//used to store scratch locations for read/write
	boost::uint64_t scratch_read_RRN = start_RRN; //read starts at beginning of row
	boost::uint64_t scratch_write_RRN = end_RRN;  //write starts at end of row

	//stopping case, if one hash passed in it is the root hash
	if(end_RRN - start_RRN == 1){
		return true;
	}

	//loop through hashes in the lower row and create the next highest row
	while(scratch_read_RRN < end_RRN){
		if(boost::this_thread::interruption_requested()){
			return false;
		}

		//hash child nodes
		int block_buff_size = 0;
		if(end_RRN - scratch_read_RRN < protocol::HASH_BLOCK_SIZE){
			//not enough hashes for full hash block
			upside_down.seekg(scratch_read_RRN * protocol::HASH_SIZE, std::ios::beg);
			upside_down.read(block_buff, (end_RRN - scratch_read_RRN) * protocol::HASH_SIZE);
			if(!upside_down.good()){
				LOGGER << "error reading scratch file";
				return false;
			}
			block_buff_size = (end_RRN - scratch_read_RRN) * protocol::HASH_SIZE;
		}else{
			//enough hashes for full hash block
			upside_down.seekg(scratch_read_RRN * protocol::HASH_SIZE, std::ios::beg);
			upside_down.read(block_buff, protocol::HASH_BLOCK_SIZE * protocol::HASH_SIZE);
			if(!upside_down.good()){
				LOGGER << "error reading scratch file";
				return false;
			}
			block_buff_size = protocol::HASH_BLOCK_SIZE * protocol::HASH_SIZE;
		}

		//create hash
		SHA.init();
		SHA.load(block_buff, block_buff_size);
		SHA.end();
		scratch_read_RRN += block_buff_size / protocol::HASH_SIZE;

		//write resulting hash
		upside_down.seekp(scratch_write_RRN * protocol::HASH_SIZE, std::ios::beg);
		upside_down.write(SHA.raw_hash(), protocol::HASH_SIZE);
		if(!upside_down.good()){
			LOGGER << "error writing scratch file";
			return false;
		}
		++scratch_write_RRN;

		if(block_buff_size < protocol::HASH_BLOCK_SIZE * protocol::HASH_SIZE){
			//last child node read incomplete, row finished
			break;
		}
	}

	//recurse
	if(!create_recurse(upside_down, rightside_up, end_RRN, scratch_write_RRN, hash,
		block_buff, SHA))
	{
		return false;
	}

	//writing the final hash tree is done depth first
	if(scratch_write_RRN - end_RRN == 1){
		//this is the recursive call with the root hash
		hash = SHA.hex_hash();
	}

	//write hashes that were passed to this function
	for(int x=start_RRN; x<end_RRN; ++x){
		upside_down.seekg(x * protocol::HASH_SIZE, std::ios::beg);
		upside_down.read(block_buff, protocol::HASH_SIZE);
		if(!upside_down.good()){
			LOGGER << "error reading scratch file";
			return false;
		}
		rightside_up.write(block_buff, protocol::HASH_SIZE);
	}
	return true; 
}

boost::uint64_t hash_tree::file_hash_to_tree_hash(boost::uint64_t row_hash_count)
{
	boost::uint64_t start_hash = row_hash_count;
	if(row_hash_count == 1){
		//root hash not included in hash tree
		return 0;
	}else if(row_hash_count % protocol::HASH_BLOCK_SIZE == 0){
		row_hash_count = start_hash / protocol::HASH_BLOCK_SIZE;
	}else{
		row_hash_count = start_hash / protocol::HASH_BLOCK_SIZE + 1;
	}
	return start_hash + file_hash_to_tree_hash(row_hash_count);
}

boost::uint64_t hash_tree::file_hash_to_tree_hash(boost::uint64_t row_hash, std::deque<boost::uint64_t> & row)
{
	boost::uint64_t start_hash = row_hash;
	if(row_hash == 1){
		//root hash not included in hash tree
		return 0;
	}else if(row_hash % protocol::HASH_BLOCK_SIZE == 0){
		row_hash = start_hash / protocol::HASH_BLOCK_SIZE;
	}else{
		row_hash = start_hash / protocol::HASH_BLOCK_SIZE + 1;
	}
	row.push_front(start_hash);
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
		return protocol::HASH_SIZE * file_hash_to_tree_hash(file_size_to_file_hash(file_size));
	}
}

hash_tree::status hash_tree::read_block(const boost::uint64_t & block_num,
	std::string & block)
{
	char block_buff[protocol::FILE_BLOCK_SIZE];
	std::pair<boost::uint64_t, unsigned> info;
	if(block_info(block_num, row, info)){
		if(!database::pool::get_proxy()->blob_read(Blob, block_buff,
			info.second, info.first))
		{
			return io_error;
		}
		block.clear();
		block.assign(block_buff, info.second);
		return good;
	}else{
		LOGGER << "invalid block number, programming error";
		exit(1);
	}
}

boost::uint64_t hash_tree::row_to_tree_block_count(const std::deque<boost::uint64_t> & row)
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

boost::uint64_t hash_tree::row_to_file_hash_offset(const std::deque<boost::uint64_t> & row)
{
	boost::uint64_t file_hash_offset = 0;
	if(row.size() == 0){
		//no hash tree, root hash is file hash
		return file_hash_offset;
	}

	//add up the size (bytes) of all rows until reaching the beginning of the last row
	for(int x=0; x<row.size()-1; ++x){
		file_hash_offset += row[x] * protocol::HASH_SIZE;
	}
	return file_hash_offset;
}

hash_tree::status hash_tree::write_block(const boost::uint64_t & block_num,
	const std::string & block, const std::string & IP)
{
	//see documentation in header for set_state_downloading
	if(set_state_downloading){
		database::table::hash::set_state(hash, database::table::hash::downloading);
		set_state_downloading = false;
	}

	std::pair<boost::uint64_t, unsigned> info;
	if(block_info(block_num, row, info)){
		if(info.second != block.size()){
			//incorrect block size
			LOGGER << "incorrect block size, programming error";
			exit(1);
		}
		if(!database::pool::get_proxy()->blob_write(Blob, block.data(),
			block.size(), info.first))
		{
			return io_error;
		}

		block_status[block_num] = 1;
		boost::uint32_t bad_block;
		status Status = check_incremental(bad_block);
		if(Status == bad && bad_block == block_num){
			database::table::blacklist::add(IP);
		}
		return Status;
	}else{
		LOGGER << "programmer error";
		exit(1);
	}
}
