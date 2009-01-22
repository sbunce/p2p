#include "hash_tree.h"

boost::uint64_t hash_tree::throw_away;

hash_tree::hash_tree()
:
	DB_Hash(DB),
	stop_thread(false)
{

}

bool hash_tree::check(tree_info & Tree_Info, boost::uint64_t & bad_block)
{
	if(Tree_Info.block_count == 0){
		return true;
	}

	for(boost::uint64_t x=0; x<Tree_Info.block_count; ++x){
		if(!check_block(Tree_Info, x)){
			LOGGER << "bad block " << x << " in tree " << Tree_Info.root_hash;
			boost::mutex::scoped_lock lock(*Tree_Info.Contiguous_mutex);
			Tree_Info.Contiguous->trim(x);
			bad_block = x;
			return false;
		}
	}
	return true;
}

bool hash_tree::check_block(tree_info & Tree_Info, const boost::uint64_t & block_num)
{
	if(Tree_Info.block_count == 0){
		//only one hash, it's the root hash and the file hash
		return true;
	}

	std::pair<boost::uint64_t, unsigned> info;
	boost::uint64_t parent;
	if(!block_info(block_num, Tree_Info.row, info, parent)){
		//invalid block sent to block_info
		LOGGER << "programmer error\n";
		exit(1);
	}

	Tree_Info.Blob.read(block_buff, info.second, info.first);

	//reserve space for speed up
	SHA.reserve(global::HASH_BLOCK_SIZE * global::HASH_SIZE);

	//create hash for children
	SHA.init();
	SHA.load(block_buff, info.second);
	SHA.end();

	//check child hash
	if(block_num == 0){
		//first row has to be checked against root hash
		return strncmp(convert::hex_to_binary(Tree_Info.root_hash).data(), SHA.raw_hash(), global::HASH_SIZE) == 0;
	}else{
		Tree_Info.Blob.read(block_buff, global::HASH_SIZE, parent);
		return strncmp(block_buff, SHA.raw_hash(), global::HASH_SIZE) == 0;
	}
}

void hash_tree::check_contiguous(tree_info & Tree_Info)
{
	boost::mutex::scoped_lock lock(*Tree_Info.Contiguous_mutex);
	contiguous_map<boost::uint64_t, std::string>::contiguous_iterator c_iter_cur, c_iter_end;
	c_iter_cur = Tree_Info.Contiguous->begin_contiguous();
	c_iter_end = Tree_Info.Contiguous->end_contiguous();
	while(c_iter_cur != c_iter_end){
		if(!check_block(Tree_Info, c_iter_cur->first)){

			#ifdef CORRUPT_HASH_BLOCK_TEST
			//rerequest only the bad block and don't blacklist
			Tree_Info.bad_block.push_back(c_iter_cur->first);
			Tree_Info.Contiguous->erase(c_iter_cur->first);
			#else

			//blacklist server that sent bad block
			LOGGER << c_iter_cur->second << " sent bad hash block " << c_iter_cur->first;
			DB_Blacklist.add(c_iter_cur->second);

			//bad block, add all blocks this server sent to bad_block
			std::vector<boost::uint64_t> bad;
			contiguous_map<boost::uint64_t, std::string>::iterator iter_cur, iter_end;
			iter_cur = Tree_Info.Contiguous->begin();
			iter_end = Tree_Info.Contiguous->end();
			while(iter_cur != iter_end){
				if(c_iter_cur->second == iter_cur->second){
					//found block that same server sent
					Tree_Info.bad_block.push_back(iter_cur->first);
					bad.push_back(iter_cur->first);
				}
				++iter_cur;
			}

			{
			//erase blocks found to be bad
			std::vector<boost::uint64_t>::iterator iter_cur, iter_end;
			iter_cur = bad.begin();
			iter_end = bad.end();
			while(iter_cur != iter_end){
				Tree_Info.Contiguous->erase(*iter_cur);
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
	SHA.reserve(global::HASH_BLOCK_SIZE * global::HASH_SIZE);

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

	boost::uint64_t tree_size = file_size_to_tree_size(file_size);
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
		Tree sucessfully created. Look in the database to see if a tree with this
		hash already exists, if it does then return the key of that tree.
		*/
		assert(tree_size == rightside_up.tellp());
		if(!DB_Hash.exists(root_hash, tree_size)){
			DB_Hash.tree_allocate(root_hash, tree_size);
			sqlite3_wrapper::blob Blob = DB_Hash.tree_open(root_hash, tree_size);
			rightside_up.seekg(0, std::ios::beg);
			//rightside_up.clear();
			int offset = 0, bytes_remaining = tree_size, read_size;
			while(bytes_remaining){
				if(bytes_remaining > global::FILE_BLOCK_SIZE){
					read_size = global::FILE_BLOCK_SIZE;
				}else{
					read_size = bytes_remaining;
				}
				rightside_up.read(block_buff, read_size);
				if(rightside_up.gcount() != read_size){
					DB_Hash.delete_tree(root_hash, tree_size);
					LOGGER << "error reading rightside_up file";
					return false;
				}else{
					Blob.write(block_buff, read_size, offset);
					offset += read_size;
					bytes_remaining -= read_size;
				}
			}
			DB_Hash.set_state(root_hash, tree_size, DB_hash::COMPLETE);
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
	Tree_Info.Blob.read(block_buff, global::HASH_SIZE, Tree_Info.file_hash_offset + file_block_num * global::HASH_SIZE);
	SHA.init();
	SHA.load(block, size);
	SHA.end();
	return strncmp(block_buff, SHA.raw_hash(), global::HASH_SIZE) == 0;
}

void hash_tree::stop()
{
	stop_thread = true;
}

bool hash_tree::read_block(tree_info & Tree_Info, const boost::uint64_t & block_num, std::string & block)
{
	std::pair<boost::uint64_t, unsigned> info;
	if(block_info(block_num, Tree_Info.row, info)){
		Tree_Info.Blob.read(block_buff, info.second, info.first);
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
	if(block_info(block_num, Tree_Info.row, info)){
		if(info.second != block.size()){
			//incorrect block size
			LOGGER << "incorrect block size, programming error";
			exit(1);
		}
		Tree_Info.Blob.write(block.data(), block.size(), info.first);
		{
		boost::mutex::scoped_lock lock(*Tree_Info.Contiguous_mutex);
		Tree_Info.Contiguous->insert(std::make_pair(block_num, IP));
		}
		check_contiguous(Tree_Info);
		return true;
	}else{
		LOGGER << "client requested invalid block number";
		DB_Blacklist.add(IP);
		return false;
	}
}
