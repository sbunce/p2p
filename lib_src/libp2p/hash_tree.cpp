#include "hash_tree.hpp"

//BEGIN hash_tree IMPLEMENTATION
hash_tree::hash_tree():
	stop_thread(false)
{

}

hash_tree::status hash_tree::check(tree_info & Tree_Info, boost::uint64_t & bad_block)
{
	if(Tree_Info.tree_block_count == 0){
		return GOOD;
	}else{
		status Status;
		for(boost::uint64_t x=0; x<Tree_Info.tree_block_count; ++x){
			Status = check_block(Tree_Info, x);
			if(Status == BAD){
				Tree_Info.Contiguous.trim(x);
				bad_block = x;
				if(bad_block != 0){
					LOGGER << "bad block " << x << " in tree " << Tree_Info.root_hash;
				}
				return BAD;
			}else if(Status == IO_ERROR){
				return IO_ERROR;
			}
		}
	}
	return GOOD;
}

hash_tree::status hash_tree::check_block(tree_info & Tree_Info, const boost::uint64_t & block_num)
{
	if(Tree_Info.tree_block_count == 0){
		//only one hash, it's the root hash and the file hash
		return GOOD;
	}

	std::pair<boost::uint64_t, unsigned> info;
	boost::uint64_t parent;
	if(!tree_info::block_info(block_num, Tree_Info.row, info, parent)){
		//invalid block sent to block_info
		LOGGER << "programmer error\n";
		exit(1);
	}

	if(!database::pool::get_proxy()->blob_read(Tree_Info.Blob, block_buff,
		info.second, info.first))
	{
		return IO_ERROR;
	}

	//create hash for children
	SHA.init();
	SHA.load(block_buff, info.second);
	SHA.end();

	//check child hash
	if(block_num == 0){
		//first row has to be checked against root hash
		std::string hash_bin;
		if(!convert::hex_to_bin(Tree_Info.root_hash, hash_bin)){
			LOGGER << "invalid hex";
			exit(1);
		}
		if(strncmp(hash_bin.data(), SHA.raw_hash(), protocol::HASH_SIZE) == 0){
			return GOOD;
		}else{
			return BAD;
		}
	}else{
		if(!database::pool::get_proxy()->blob_read(Tree_Info.Blob, block_buff,
			protocol::HASH_SIZE, parent))
		{
			return IO_ERROR;
		}

		if(strncmp(block_buff, SHA.raw_hash(), protocol::HASH_SIZE) == 0){
			return GOOD;
		}else{
			return BAD;
		}
	}
}

hash_tree::status hash_tree::check_file_block(tree_info & Tree_Info, const boost::uint64_t & file_block_num, const char * block, const int & size)
{
	char parent_buff[protocol::HASH_SIZE];
	if(!database::pool::get_proxy()->blob_read(Tree_Info.Blob, parent_buff,
		protocol::HASH_SIZE, Tree_Info.file_hash_offset + file_block_num * protocol::HASH_SIZE))
	{
		return IO_ERROR;
	}
	SHA.init();
	SHA.load(block, size);
	SHA.end();
	if(strncmp(parent_buff, SHA.raw_hash(), protocol::HASH_SIZE) == 0){
		return GOOD;
	}else{
		return BAD;
	}
}

void hash_tree::check_contiguous(tree_info & Tree_Info)
{
	for(contiguous_map<boost::uint64_t, std::string>::contiguous_iterator
		c_iter_cur = Tree_Info.Contiguous.begin_contiguous(),
		c_iter_end = Tree_Info.Contiguous.end_contiguous();
		c_iter_cur != c_iter_end; ++c_iter_cur)
	{
		status Status = check_block(Tree_Info, c_iter_cur->first);
		if(Status == BAD){
			#ifdef CORRUPT_HASH_BLOCK_TEST
			//rerequest only the bad block and don't blacklist
			Tree_Info.bad_block.push_back(c_iter_cur->first);
			Tree_Info.Contiguous->erase(c_iter_cur->first);
			#else

			//blacklist server that sent bad block
			LOGGER << c_iter_cur->second << " sent bad hash block " << c_iter_cur->first;
			database::table::blacklist::add(c_iter_cur->second);

			//bad block, add all blocks this server sent to bad_block
			contiguous_map<boost::uint64_t, std::string>::iterator iter_cur, iter_end;
			iter_cur = Tree_Info.Contiguous.begin();
			iter_end = Tree_Info.Contiguous.end();
			while(iter_cur != iter_end){
				if(c_iter_cur->second == iter_cur->second){
					//found block that same server sent
					Tree_Info.bad_block.push_back(iter_cur->first);
					Tree_Info.Contiguous.erase(iter_cur++);
				}else{
					++iter_cur;
				}
			}
			#endif
			break;
		}else if(Status == IO_ERROR){
			/*
			IO_ERROR can't be communicated back to download_hash_tree from here.
			However, we can wait until download_hash_tree tries to write and it
			should error out then.
			*/
			break;
		}
	}

	//any contiguous blocks left were checked and can now be removed
	Tree_Info.Contiguous.trim_contiguous();
}

bool hash_tree::create(const std::string & file_path, const boost::uint64_t & expected_file_size,
	std::string & root_hash)
{
	std::fstream fin(file_path.c_str(), std::ios::in | std::ios::binary);
	if(!fin.good()){
		LOGGER << "error opening file " << file_path;
		return false;
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
	std::fstream upside_down(path::upside_down().c_str(), std::ios::in | std::ios::out | std::ios::trunc | std::ios::binary);
	if(!upside_down.good()){
		LOGGER << "error opening upside_down file";
		return false;
	}
	std::fstream rightside_up(path::rightside_up().c_str(), std::ios::in | std::ios::out | std::ios::trunc | std::ios::binary);
	if(!rightside_up.good()){
		LOGGER << "error opening rightside_up file";
		return false;
	}

	//create all file block hashes
	boost::uint64_t blocks_read = 0;
	boost::uint64_t file_size = 0;
	while(true){
		if(boost::this_thread::interruption_requested()){
			return false;
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
			return false;
		}
		++blocks_read;
		file_size += fin.gcount();

		if(stop_thread || file_size > expected_file_size){
			return false;
		}
	}

	if(fin.bad() || !fin.eof()){
		LOGGER << "error reading file \"" << file_path << "\"";
		return false;
	}

	if(blocks_read == 0){
		//do not generate hash trees for empty files
		return false;
	}

	//base case, the file size is <= one block
	if(blocks_read == 1){
		root_hash = SHA.hex_hash();
		return true;
	}

	boost::uint64_t tree_size = tree_info::file_size_to_tree_size(file_size);
	if(tree_size > std::numeric_limits<int>::max()){
		LOGGER << "file at location \"" << file_path << "\" would generate hash tree beyond max SQLite3 blob size";
		return false;
	}

	create_recurse(upside_down, rightside_up, 0, blocks_read, root_hash);

	if(root_hash.empty()){
		//tree didn't generate, i/o error or cancelled with stop()
		return false;
	}else{
		/*
		Tree sucessfully created. Look in the database to see if the tree already
		exists in the hash table. If it does there's no reason to replace it with
		the tree just generated.

		Note: The state of the hash tree is only set to reserved after this
		finishes. Meaning if the tree is generated and the program is interrupted
		before the tree is referenced somewhere, the tree will be deleted upon
		program start. (think garbage collection)

		Whenever a reference is made to a tree the state of the hash tree should
		be changed to complete.
		*/
		assert(tree_size == rightside_up.tellp());
		database::pool::proxy DB;
		if(!database::table::hash::exists(root_hash, tree_size, DB)){
			/*
			This tree does not already exist in the database. The tree needs to be
			copied from the temporary rightside_up file, in to the blob (where the
			hash tree goes).

			The transaction guarantees that the tree is either entirely there, or
			not there at all.
			*/
			DB->query("BEGIN TRANSACTION");
			//allocate blob for new hash tree
			if(!database::table::hash::tree_allocate(root_hash, tree_size, DB)){
				LOGGER << "could not allocate blob for hash tree";
				DB->query("END TRANSACTION");
				return false;
			}
			database::blob Blob = database::table::hash::tree_open(root_hash, tree_size, DB);
			rightside_up.seekg(0, std::ios::beg);
			int offset = 0, bytes_remaining = tree_size, read_size;
			while(bytes_remaining){
				if(boost::this_thread::interruption_requested()){
					database::table::hash::delete_tree(root_hash, tree_size, DB);
					DB->query("END TRANSACTION");
					return false;
				}

				if(bytes_remaining > protocol::FILE_BLOCK_SIZE){
					read_size = protocol::FILE_BLOCK_SIZE;
				}else{
					read_size = bytes_remaining;
				}
				rightside_up.read(block_buff, read_size);
				if(rightside_up.gcount() != read_size){
					LOGGER << "error reading rightside_up file";
					database::table::hash::delete_tree(root_hash, tree_size, DB);
					DB->query("END TRANSACTION");
					return false;
				}else{
					if(!DB->blob_write(Blob, block_buff, read_size, offset)){
						LOGGER << "error doing incremental write to blob";
						database::table::hash::delete_tree(root_hash, tree_size, DB);
						DB->query("END TRANSACTION");
						return false;
					}
					offset += read_size;
					bytes_remaining -= read_size;
				}
			}
			DB->query("END TRANSACTION");
		}
		return true;
	}
}

bool hash_tree::create_recurse(std::fstream & upside_down, std::fstream & rightside_up,
	boost::uint64_t start_RRN, boost::uint64_t end_RRN, std::string & root_hash)
{
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

		if(stop_thread){
			return false;
		}
	}

	//recurse
	if(!create_recurse(upside_down, rightside_up, end_RRN, scratch_write_RRN, root_hash)){
		return false;
	}

	/*
	Writing of the result hash tree file is depth first.
	*/
	if(scratch_write_RRN - end_RRN == 1){
		//this is the recursive call with the root hash
		root_hash = SHA.hex_hash();
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

void hash_tree::stop()
{
	stop_thread = true;
}

hash_tree::status hash_tree::read_block(tree_info & Tree_Info, const boost::uint64_t & block_num, std::string & block)
{
	char block_buff[protocol::FILE_BLOCK_SIZE];
	std::pair<boost::uint64_t, unsigned> info;
	if(tree_info::block_info(block_num, Tree_Info.row, info)){
		if(!database::pool::get_proxy()->blob_read(Tree_Info.Blob, block_buff,
			info.second, info.first))
		{
			return IO_ERROR;
		}
		block.clear();
		block.assign(block_buff, info.second);
		return GOOD;
	}else{
		LOGGER << "invalid block number, programming error";
		exit(1);
	}
}

hash_tree::status hash_tree::write_block(tree_info & Tree_Info, const boost::uint64_t & block_num, const std::string & block, const std::string & IP)
{
	std::pair<boost::uint64_t, unsigned> info;
	if(tree_info::block_info(block_num, Tree_Info.row, info)){
		if(info.second != block.size()){
			//incorrect block size
			LOGGER << "incorrect block size, programming error";
			exit(1);
		}
		if(!database::pool::get_proxy()->blob_write(Tree_Info.Blob, block.data(),
			block.size(), info.first))
		{
			return IO_ERROR;
		}
		Tree_Info.Contiguous.insert(std::make_pair(block_num, IP));
		check_contiguous(Tree_Info);
		return GOOD;
	}else{
		LOGGER << "programmer error";
		exit(1);
	}
}
//END hash_tree IMPLEMENTATION

//BEGIN hash_tree::tree_info IMPLEMENTATION
hash_tree::tree_info::tree_info(
	const std::string & root_hash_in,
	const boost::uint64_t & file_size_in
):
	root_hash(root_hash_in),
	file_size(file_size_in),
	Blob(database::table::hash::tree_open(root_hash_in, file_size_to_tree_size(file_size_in)))
{
	file_size_to_tree_hash(file_size, row);
	tree_block_count = row_to_tree_block_count(row);
	file_hash_offset = row_to_file_hash_offset(row);
	tree_size = file_size_to_tree_size(file_size);
	Contiguous.set_range(0, tree_block_count);

	if(file_size % protocol::FILE_BLOCK_SIZE == 0){
		file_block_count = file_size / protocol::FILE_BLOCK_SIZE;
	}else{
		file_block_count = file_size / protocol::FILE_BLOCK_SIZE + 1;
	}

	if(file_size % protocol::FILE_BLOCK_SIZE == 0){
		last_file_block_size = protocol::FILE_BLOCK_SIZE;
	}else{
		last_file_block_size = file_size % protocol::FILE_BLOCK_SIZE;
	}
}

bool hash_tree::tree_info::block_info(const boost::uint64_t & block, const std::deque<boost::uint64_t> & row,
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

bool hash_tree::tree_info::block_info(const boost::uint64_t & block, const std::deque<boost::uint64_t> & row,
	std::pair<boost::uint64_t, unsigned> & info)
{
	boost::uint64_t throw_away;
	return block_info(block, row, info, throw_away);
}

unsigned hash_tree::tree_info::block_size(const boost::uint64_t & block_num)
{
	std::pair<boost::uint64_t, unsigned> info;
	if(block_info(block_num, row, info)){
		return info.second;
	}else{
		LOGGER << "programmer error, invalid block specified";
		exit(1);
	}
}

boost::uint64_t hash_tree::tree_info::file_hash_to_tree_hash(boost::uint64_t row_hash_count)
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

boost::uint64_t hash_tree::tree_info::file_hash_to_tree_hash(boost::uint64_t row_hash, std::deque<boost::uint64_t> & row)
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

boost::uint64_t hash_tree::tree_info::file_size_to_file_hash(boost::uint64_t file_size)
{
	boost::uint64_t hash_count = file_size / protocol::FILE_BLOCK_SIZE;
	if(file_size % protocol::FILE_BLOCK_SIZE != 0){
		//add one for partial last block
		++hash_count;
	}
	return hash_count;
}

boost::uint64_t hash_tree::tree_info::file_size_to_tree_hash(const boost::uint64_t & file_size, std::deque<boost::uint64_t> & row)
{
	return file_hash_to_tree_hash(file_size_to_file_hash(file_size), row);
}

boost::uint64_t hash_tree::tree_info::file_size_to_tree_size(const boost::uint64_t & file_size)
{
	return protocol::HASH_SIZE * file_hash_to_tree_hash(file_size_to_file_hash(file_size));
}

const std::string & hash_tree::tree_info::get_root_hash()
{
	return root_hash;
}

const boost::uint64_t & hash_tree::tree_info::get_tree_block_count()
{
	return tree_block_count;
}

const boost::uint64_t & hash_tree::tree_info::get_tree_size()
{
	return tree_size;
}

const boost::uint64_t & hash_tree::tree_info::get_file_block_count()
{
	return file_block_count;
}

const boost::uint64_t & hash_tree::tree_info::get_file_size()
{
	return file_size;
}

const boost::uint64_t & hash_tree::tree_info::get_last_file_block_size()
{
	return last_file_block_size;
}

bool hash_tree::tree_info::highest_good(boost::uint64_t & HG)
{
	HG = Contiguous.start_range();
	if(HG != 0){
		--HG;
		return true;
	}else{
		return false;
	}
}

/*
boost::uint64_t hash_tree::tree_info::rerequest_bad_blocks(request_generator & Request_Generator)
{
	boost::uint64_t size = 0;
	for(std::vector<boost::uint64_t>::iterator iter_cur = bad_block.begin(),
		iter_end = bad_block.end(); iter_cur != iter_end; ++iter_cur)
	{
		size += block_size(*iter_cur);
		Request_Generator.force_rerequest(*iter_cur);
	}
	bad_block.clear();
	return size;
}
*/
boost::uint64_t hash_tree::tree_info::row_to_tree_block_count(const std::deque<boost::uint64_t> & row)
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

boost::uint64_t hash_tree::tree_info::row_to_file_hash_offset(const std::deque<boost::uint64_t> & row)
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
//END hash_tree::tree_info IMPLEMENTATION
