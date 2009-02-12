#include "hash_tree.hpp"

//BEGIN hash_tree IMPLEMENTATION
hash_tree::hash_tree()
: stop_thread(false)
{

}

bool hash_tree::check(tree_info & Tree_Info, boost::uint64_t & bad_block)
{
	if(Tree_Info.block_count == 0){
		return true;
	}else{

		sha SHA;
		SHA.reserve(global::HASH_BLOCK_SIZE * global::HASH_SIZE);
		char block_buff[global::FILE_BLOCK_SIZE];

		for(boost::uint64_t x=0; x<Tree_Info.block_count; ++x){
			if(!check_block(Tree_Info, x, SHA, block_buff)){
				boost::recursive_mutex::scoped_lock lock(*Tree_Info.Recursive_Mutex);
				Tree_Info.Contiguous->trim(x);
				bad_block = x;
				LOGGER << "bad block " << x << " in tree " << Tree_Info.root_hash;
				return false;
			}
		}
	}
	return true;
}

bool hash_tree::check_block(tree_info & Tree_Info, const boost::uint64_t & block_num, sha & SHA, char * block_buff)
{
	if(Tree_Info.block_count == 0){
		//only one hash, it's the root hash and the file hash
		return true;
	}

	std::pair<boost::uint64_t, unsigned> info;
	boost::uint64_t parent;
	if(!tree_info::block_info(block_num, Tree_Info.row, info, parent)){
		//invalid block sent to block_info
		LOGGER << "programmer error\n";
		exit(1);
	}

	DB.blob_read(Tree_Info.Blob, block_buff, info.second, info.first);

	//create hash for children
	SHA.init();
	SHA.load(block_buff, info.second);
	SHA.end();

	//check child hash
	if(block_num == 0){
		//first row has to be checked against root hash
		return strncmp(convert::hex_to_bin(Tree_Info.root_hash).data(), SHA.raw_hash(), global::HASH_SIZE) == 0;
	}else{
		DB.blob_read(Tree_Info.Blob, block_buff, global::HASH_SIZE, parent);
		return strncmp(block_buff, SHA.raw_hash(), global::HASH_SIZE) == 0;
	}
}

void hash_tree::check_contiguous(tree_info & Tree_Info)
{
	boost::recursive_mutex::scoped_lock lock(*Tree_Info.Recursive_Mutex);

	sha SHA;
	SHA.reserve(global::HASH_BLOCK_SIZE * global::HASH_SIZE);
	char block_buff[global::FILE_BLOCK_SIZE];

	contiguous_map<boost::uint64_t, std::string>::contiguous_iterator c_iter_cur, c_iter_end;
	c_iter_cur = Tree_Info.Contiguous->begin_contiguous();
	c_iter_end = Tree_Info.Contiguous->end_contiguous();
	while(c_iter_cur != c_iter_end){
		if(!check_block(Tree_Info, c_iter_cur->first, SHA, block_buff)){

			#ifdef CORRUPT_HASH_BLOCK_TEST
			//rerequest only the bad block and don't blacklist
			Tree_Info.bad_block.push_back(c_iter_cur->first);
			Tree_Info.Contiguous->erase(c_iter_cur->first);
			#else

			//blacklist server that sent bad block
			LOGGER << c_iter_cur->second << " sent bad hash block " << c_iter_cur->first;
			DB_Blacklist.add(c_iter_cur->second);

			//bad block, add all blocks this server sent to bad_block
			contiguous_map<boost::uint64_t, std::string>::iterator iter_cur, iter_end;
			iter_cur = Tree_Info.Contiguous->begin();
			iter_end = Tree_Info.Contiguous->end();
			while(iter_cur != iter_end){
				if(c_iter_cur->second == iter_cur->second){
					//found block that same server sent
					Tree_Info.bad_block.push_back(iter_cur->first);
					Tree_Info.Contiguous->erase(iter_cur++);
				}else{
					++iter_cur;
				}
			}
			#endif
			break;
		}
		++c_iter_cur;
	}

	//any contiguous blocks left were checked and can now be removed
	Tree_Info.Contiguous->trim_contiguous();
}

bool hash_tree::create(const std::string & file_path, std::string & root_hash)
{
	sha SHA;
	SHA.reserve(global::HASH_BLOCK_SIZE * global::HASH_SIZE);
	char block_buff[global::FILE_BLOCK_SIZE];

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
	std::fstream upside_down("upside_down", std::ios::in | std::ios::out | std::ios::trunc | std::ios::binary);
	if(!upside_down.good()){
		LOGGER << "error opening upside_down file";
		return false;
	}
	std::fstream rightside_up("rightside_up", std::ios::in | std::ios::out | std::ios::trunc | std::ios::binary);
	if(!rightside_up.good()){
		LOGGER << "error opening rightside_up file";
		return false;
	}

	//create all file block hashes
	boost::uint64_t blocks_read = 0;
	boost::uint64_t file_size = 0;
	while(true){
		fin.read(block_buff, global::FILE_BLOCK_SIZE);
		if(fin.gcount() == 0){
			break;
		}

		SHA.init();
		SHA.load(block_buff, fin.gcount());
		SHA.end();
		upside_down.write(SHA.raw_hash(), global::HASH_SIZE);
		if(!upside_down.good()){
			LOGGER << "error writing to upside_down file";
			return false;
		}
		++blocks_read;
		file_size += fin.gcount();

		if(stop_thread){
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

	create_recurse(upside_down, rightside_up, 0, blocks_read, root_hash, SHA, block_buff);

	if(root_hash.empty()){
		//tree didn't generate, i/o error or cancelled with stop()
		return false;
	}else{
		/*
		Tree sucessfully created. Look in the database to see if a tree with this
		hash already exists, if it does then return the key of that tree.
		*/
		assert(tree_size == rightside_up.tellp());
		if(!database::table::hash::exists(root_hash, tree_size, DB)){
			database::table::hash::tree_allocate(root_hash, tree_size, DB);
			database::blob Blob = database::table::hash::tree_open(root_hash, tree_size, DB);
			rightside_up.seekg(0, std::ios::beg);
			int offset = 0, bytes_remaining = tree_size, read_size;
			DB.query("BEGIN TRANSACTION");
			while(bytes_remaining){
				if(bytes_remaining > global::FILE_BLOCK_SIZE){
					read_size = global::FILE_BLOCK_SIZE;
				}else{
					read_size = bytes_remaining;
				}
				rightside_up.read(block_buff, read_size);
				if(rightside_up.gcount() != read_size){
					database::table::hash::delete_tree(root_hash, tree_size, DB);
					LOGGER << "error reading rightside_up file";
					return false;
				}else{
					DB.blob_write(Blob, block_buff, read_size, offset);
					offset += read_size;
					bytes_remaining -= read_size;
				}
			}
			database::table::hash::set_state(root_hash, tree_size, database::table::hash::COMPLETE, DB);
			DB.query("END TRANSACTION");
		}
		return true;
	}
}

bool hash_tree::create_recurse(std::fstream & upside_down, std::fstream & rightside_up,
	boost::uint64_t start_RRN, boost::uint64_t end_RRN, std::string & root_hash, sha & SHA, char * block_buff)
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
		//hash child nodes
		int block_buff_size = 0;
		if(end_RRN - scratch_read_RRN < global::HASH_BLOCK_SIZE){
			//not enough hashes for full hash block
			upside_down.seekg(scratch_read_RRN * global::HASH_SIZE, std::ios::beg);
			upside_down.read(block_buff, (end_RRN - scratch_read_RRN) * global::HASH_SIZE);
			if(!upside_down.good()){
				LOGGER << "error reading scratch file";
				return false;
			}
			block_buff_size = (end_RRN - scratch_read_RRN) * global::HASH_SIZE;
		}else{
			//enough hashes for full hash block
			upside_down.seekg(scratch_read_RRN * global::HASH_SIZE, std::ios::beg);
			upside_down.read(block_buff, global::HASH_BLOCK_SIZE * global::HASH_SIZE);
			if(!upside_down.good()){
				LOGGER << "error reading scratch file";
				return false;
			}
			block_buff_size = global::HASH_BLOCK_SIZE * global::HASH_SIZE;
		}

		//create hash
		SHA.init();
		SHA.load(block_buff, block_buff_size);
		SHA.end();
		scratch_read_RRN += block_buff_size / global::HASH_SIZE;

		//write resulting hash
		upside_down.seekp(scratch_write_RRN * global::HASH_SIZE, std::ios::beg);
		upside_down.write(SHA.raw_hash(), global::HASH_SIZE);
		if(!upside_down.good()){
			LOGGER << "error writing scratch file";
			return false;
		}
		++scratch_write_RRN;

		if(block_buff_size < global::HASH_BLOCK_SIZE * global::HASH_SIZE){
			//last child node read incomplete, row finished
			break;
		}

		if(stop_thread){
			return false;
		}
	}

	//recurse
	if(!create_recurse(upside_down, rightside_up, end_RRN, scratch_write_RRN, root_hash, SHA, block_buff)){
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
		upside_down.seekg(x * global::HASH_SIZE, std::ios::beg);
		upside_down.read(block_buff, global::HASH_SIZE);
		if(!upside_down.good()){
			LOGGER << "error reading scratch file";
			return false;
		}
		rightside_up.write(block_buff, global::HASH_SIZE);
	}
	return true; 
}

bool hash_tree::check_file_block(tree_info & Tree_Info, const boost::uint64_t & file_block_num, const char * block, const int & size)
{
	sha SHA;
	char parent_buff[global::HASH_SIZE];
	DB.blob_read(Tree_Info.Blob, parent_buff, global::HASH_SIZE, Tree_Info.file_hash_offset + file_block_num * global::HASH_SIZE);
	SHA.init();
	SHA.load(block, size);
	SHA.end();
	return strncmp(parent_buff, SHA.raw_hash(), global::HASH_SIZE) == 0;
}

void hash_tree::stop()
{
	stop_thread = true;
}

bool hash_tree::read_block(tree_info & Tree_Info, const boost::uint64_t & block_num, std::string & block)
{
	char block_buff[global::FILE_BLOCK_SIZE];
	std::pair<boost::uint64_t, unsigned> info;
	if(tree_info::block_info(block_num, Tree_Info.row, info)){
		DB.blob_read(Tree_Info.Blob, block_buff, info.second, info.first);
		block.clear();
		block.assign(block_buff, info.second);
		return true;
	}else{
		LOGGER << "invalid block number, programming error";
		exit(1);
	}
}

bool hash_tree::write_block(tree_info & Tree_Info, const boost::uint64_t & block_num, const std::string & block, const std::string & IP)
{
	std::pair<boost::uint64_t, unsigned> info;
	if(tree_info::block_info(block_num, Tree_Info.row, info)){
		if(info.second != block.size()){
			//incorrect block size
			LOGGER << "incorrect block size, programming error";
			exit(1);
		}
		DB.blob_write(Tree_Info.Blob, block.data(), block.size(), info.first);
		Tree_Info.Recursive_Mutex->lock();
		Tree_Info.Contiguous->insert(std::make_pair(block_num, IP));
		Tree_Info.Recursive_Mutex->unlock();
		check_contiguous(Tree_Info);
		return true;
	}else{
		LOGGER << "client requested invalid block number";
		DB_Blacklist.add(IP);
		return false;
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
	Blob(database::table::hash::tree_open(root_hash_in, file_size_to_tree_size(file_size_in))),
	Recursive_Mutex(new boost::recursive_mutex())
{
	file_size_to_tree_hash(file_size, row);
	block_count = row_to_block_count(row);
	file_hash_offset = row_to_file_hash_offset(row);
	tree_size = file_size_to_tree_size(file_size);

	Contiguous = boost::shared_ptr<contiguous_map<boost::uint64_t,
		std::string> >(new contiguous_map<boost::uint64_t, std::string>(0, block_count));

	if(file_size % global::FILE_BLOCK_SIZE == 0){
		file_block_count = file_size / global::FILE_BLOCK_SIZE;
	}else{
		file_block_count = file_size / global::FILE_BLOCK_SIZE + 1;
	}

	if(file_size % global::FILE_BLOCK_SIZE == 0){
		last_file_block_size = global::FILE_BLOCK_SIZE;
	}else{
		last_file_block_size = file_size % global::FILE_BLOCK_SIZE;
	}
}

bool hash_tree::tree_info::block_info(const boost::uint64_t & block, const std::deque<boost::uint64_t> & row,
	std::pair<boost::uint64_t, unsigned> & info, boost::uint64_t & parent)
{
	boost::uint64_t offset = 0;          //hash offset from beginning of file to start of row
	boost::uint64_t block_count = 0;     //total block count in all previous rows
	boost::uint64_t row_block_count = 0; //total block count in current row
	for(unsigned x=0; x<row.size(); ++x){
		if(row[x] % global::HASH_BLOCK_SIZE == 0){
			row_block_count = row[x] / global::HASH_BLOCK_SIZE;
		}else{
			row_block_count = row[x] / global::HASH_BLOCK_SIZE + 1;
		}

		if(block_count + row_block_count > block){
			//end of row greater than block we're looking for, block exists in row
			info.first = offset + (block - block_count) * global::HASH_BLOCK_SIZE;

			//hashes between offset and end of current row
			boost::uint64_t delta = offset + row[x] - info.first;

			//determine size of block
			if(delta > global::HASH_BLOCK_SIZE){
				//full hash block
				info.second = global::HASH_BLOCK_SIZE;
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
			info.first = info.first * global::HASH_SIZE;
			info.second = info.second * global::HASH_SIZE;
			parent = parent * global::HASH_SIZE;

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
	static boost::uint64_t throw_away;
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
	}else if(row_hash_count % global::HASH_BLOCK_SIZE == 0){
		row_hash_count = start_hash / global::HASH_BLOCK_SIZE;
	}else{
		row_hash_count = start_hash / global::HASH_BLOCK_SIZE + 1;
	}
	return start_hash + file_hash_to_tree_hash(row_hash_count);
}

boost::uint64_t hash_tree::tree_info::file_hash_to_tree_hash(boost::uint64_t row_hash, std::deque<boost::uint64_t> & row)
{
	boost::uint64_t start_hash = row_hash;
	if(row_hash == 1){
		//root hash not included in hash tree
		return 0;
	}else if(row_hash % global::HASH_BLOCK_SIZE == 0){
		row_hash = start_hash / global::HASH_BLOCK_SIZE;
	}else{
		row_hash = start_hash / global::HASH_BLOCK_SIZE + 1;
	}
	row.push_front(start_hash);
	return start_hash + file_hash_to_tree_hash(row_hash, row);
}

boost::uint64_t hash_tree::tree_info::file_size_to_file_hash(boost::uint64_t file_size)
{
	boost::uint64_t hash_count = file_size / global::FILE_BLOCK_SIZE;
	if(file_size % global::FILE_BLOCK_SIZE != 0){
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
	return global::HASH_SIZE * file_hash_to_tree_hash(file_size_to_file_hash(file_size));
}

const std::string & hash_tree::tree_info::get_root_hash()
{
	return root_hash;
}

const boost::uint64_t & hash_tree::tree_info::get_block_count()
{
	return block_count;
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

const std::deque<boost::uint64_t> & hash_tree::tree_info::get_row()
{
	return row;
}

bool hash_tree::tree_info::highest_good(boost::uint64_t & HG)
{
	boost::recursive_mutex::scoped_lock lock(*Recursive_Mutex);
	HG = Contiguous->start_range();
	if(HG != 0){
		--HG;
		return true;
	}else{
		return false;
	}
}

void hash_tree::tree_info::rerequest_bad_blocks(request_generator & Request_Generator)
{
	boost::recursive_mutex::scoped_lock lock(*Recursive_Mutex);
	std::vector<boost::uint64_t>::iterator iter_cur, iter_end;
	iter_cur = bad_block.begin();
	iter_end = bad_block.end();
	while(iter_cur != iter_end){
		Request_Generator.force_rerequest(*iter_cur);
		++iter_cur;
	}
	bad_block.clear();
}

boost::uint64_t hash_tree::tree_info::row_to_block_count(const std::deque<boost::uint64_t> & row)
{
	boost::uint64_t block_count = 0;
	for(int x=0; x<row.size(); ++x){
		if(row[x] % global::HASH_BLOCK_SIZE == 0){
			block_count += row[x] / global::HASH_BLOCK_SIZE;
		}else{
			block_count += row[x] / global::HASH_BLOCK_SIZE + 1;
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
		file_hash_offset += row[x] * global::HASH_SIZE;
	}
	return file_hash_offset;
}
//END hash_tree::tree_info IMPLEMENTATION
