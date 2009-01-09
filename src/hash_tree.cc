#include "hash_tree.h"

boost::uint64_t hash_tree::throw_away;

hash_tree::hash_tree():
	stop_thread(false)
{
	//create hash directory if it doesn't already exist
	boost::filesystem::create_directory(global::HASH_DIRECTORY.c_str());
}

bool hash_tree::check(tree_info & Tree_Info, boost::uint64_t & bad_block)
{
	if(Tree_Info.block_count == 0){
		return true;
	}

	//create empty file for hash tree if it doesn't exist
	std::fstream fin((global::HASH_DIRECTORY+Tree_Info.root_hash).c_str(), std::ios::in);
	if(!fin.is_open()){
		//hash tree does not exist yet
		fin.open((global::HASH_DIRECTORY+Tree_Info.root_hash).c_str(), std::ios::out);
		bad_block = 0;
		return false;
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

bool hash_tree::check_block(const tree_info & Tree_Info, const boost::uint64_t & block_num)
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

	std::fstream fin((global::HASH_DIRECTORY + Tree_Info.root_hash).c_str(), std::ios::in | std::ios::binary);
	if(!fin.is_open()){
		LOGGER << "cannot open hash tree " << Tree_Info.root_hash;
		exit(1);
	}

	fin.seekg(info.first, std::ios::beg);
	fin.read(block_buff, info.second);
	if(fin.gcount() != info.second){
		return false;
	}

	//reserve space for speed up
	SHA.reserve(global::HASH_BLOCK_SIZE * sha::HASH_SIZE);

	//create hash for children
	SHA.init();
	SHA.load(block_buff, info.second);
	SHA.end();

	//check child hash
	if(block_num == 0){
		//first row has to be checked against root hash
		return strncmp(convert::hex_to_binary(Tree_Info.root_hash).data(), SHA.raw_hash(), sha::HASH_SIZE) == 0;
	}else{
		//read parent hash
		fin.seekg(parent, std::ios::beg);
		fin.read(block_buff, sha::HASH_SIZE);
		if(!fin.good()){
			return false;
		}

		return strncmp(block_buff, SHA.raw_hash(), sha::HASH_SIZE) == 0;
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
	//reserve space for speed up
	SHA.reserve(global::HASH_BLOCK_SIZE * sha::HASH_SIZE);

	//open file to hash, and create scratch file in which to build hash tree
	std::fstream fin(file_path.c_str(), std::ios::in | std::ios::binary);
	if(!fin.good()){
		LOGGER << "error opening file " << file_path;
		return false;
	}
	std::fstream scratch((global::HASH_DIRECTORY+"upside_down").c_str(), std::ios::in
		| std::ios::out | std::ios::trunc | std::ios::binary);
	if(!scratch.good()){
		LOGGER << "error opening scratch file";
		return false;
	}

	//create all file block hashes
	boost::uint64_t blocks_read = 0;
	while(true){
		fin.read(block_buff, global::FILE_BLOCK_SIZE);
		if(fin.gcount() == 0){
			break;
		}

		SHA.init();
		SHA.load(block_buff, fin.gcount());
		SHA.end();
		scratch.write(SHA.raw_hash(), sha::HASH_SIZE);
		if(!scratch.good()){
			LOGGER << "error writing to scratch file";
			return false;
		}
		++blocks_read;

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
		std::remove((global::HASH_DIRECTORY+"upside_down").c_str());
		return true;
	}

	//start recursive tree generating function
	create_recurse(scratch, 0, blocks_read, root_hash);
	scratch.close();

	//remove the upside down tree, rename rightside up tree to root_hash
	std::remove((global::HASH_DIRECTORY+"upside_down").c_str());
	std::rename((global::HASH_DIRECTORY+"rightside_up").c_str(), (global::HASH_DIRECTORY+root_hash).c_str());

	//root hash may be empty if hashing stopped with stop()
	return !root_hash.empty();
}

bool hash_tree::create_recurse(std::fstream & scratch, boost::uint64_t start_RRN,
	boost::uint64_t end_RRN, std::string & root_hash)
{
	//general purpose, used for row offsets
	unsigned offset = 0;

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
			scratch.seekg(scratch_read_RRN * sha::HASH_SIZE, std::ios::beg);
			scratch.read(block_buff, (end_RRN - scratch_read_RRN) * sha::HASH_SIZE);
			if(!scratch.good()){
				LOGGER << "error reading scratch file";
				return false;
			}
			block_buff_size = (end_RRN - scratch_read_RRN) * sha::HASH_SIZE;
		}else{
			//enough hashes for full hash block
			scratch.seekg(scratch_read_RRN * sha::HASH_SIZE, std::ios::beg);
			scratch.read(block_buff, global::HASH_BLOCK_SIZE * sha::HASH_SIZE);
			if(!scratch.good()){
				LOGGER << "error reading scratch file";
				return false;
			}
			block_buff_size = global::HASH_BLOCK_SIZE * sha::HASH_SIZE;
		}

		//create hash
		SHA.init();
		SHA.load(block_buff, block_buff_size);
		SHA.end();
		scratch_read_RRN += block_buff_size / sha::HASH_SIZE;

		//write resulting hash
		scratch.seekp(scratch_write_RRN * sha::HASH_SIZE, std::ios::beg);
		scratch.write(SHA.raw_hash(), sha::HASH_SIZE);
		if(!scratch.good()){
			LOGGER << "error writing scratch file";
			return false;
		}
		++scratch_write_RRN;

		if(block_buff_size < global::HASH_BLOCK_SIZE * sha::HASH_SIZE){
			//last child node read incomplete, row finished
			break;
		}

		if(stop_thread){
			return false;
		}
	}

	//recurse
	if(!create_recurse(scratch, end_RRN, scratch_write_RRN, root_hash)){
		return false;
	}

	/*
	Writing of the result hash tree file is depth first, the root node will be
	added first.
	*/
	std::fstream fout;
	if(scratch_write_RRN - end_RRN == 1){
		//this is the recursive call with the root hash
		root_hash = SHA.hex_hash();

		//do nothing if hash tree file already created
		std::fstream fin((global::HASH_DIRECTORY+"rightside_up").c_str(), std::ios::in);
		if(fin.is_open()){
			return true;
		}

		//create the file with the root hash as the name
		fout.open((global::HASH_DIRECTORY+"rightside_up").c_str(), std::ios::out | std::ios::app | std::ios::binary);
		if(!scratch.good()){
			LOGGER << "error opening rightside_up file";
			return false;
		}
	}else{
		//this is not the recursive call with the root hash
		fout.open((global::HASH_DIRECTORY+"rightside_up").c_str(), std::ios::out | std::ios::app | std::ios::binary);
		if(!scratch.good()){
			LOGGER << "error opening rightside up file";
			return false;
		}
	}

	//write hashes that were passed to this function
	for(int x=start_RRN; x<end_RRN; ++x){
		scratch.seekg(x * sha::HASH_SIZE, std::ios::beg);
		scratch.read(block_buff, sha::HASH_SIZE);
		if(!scratch.good()){
			LOGGER << "error reading scratch file";
			return false;
		}
		fout.write(block_buff, sha::HASH_SIZE);
	}
	return true;
}

bool hash_tree::check_file_block(const tree_info & Tree_Info, const boost::uint64_t & file_block_num, const char * block, const int & size)
{
	std::fstream fin((global::HASH_DIRECTORY + Tree_Info.root_hash).c_str(), std::ios::in | std::ios::binary);
	if(!fin.is_open()){
		LOGGER << "error reading hash tree " << Tree_Info.root_hash;
		exit(1);
	}else{
		//read file hash
		fin.seekg(Tree_Info.file_hash_offset + file_block_num * sha::HASH_SIZE, std::ios::beg);
		fin.read(block_buff, sha::HASH_SIZE);

		//hash file block
		SHA.init();
		SHA.load(block, size);
		SHA.end();

		//validate
		return strncmp(block_buff, SHA.raw_hash(), sha::HASH_SIZE) == 0;
	}
}

void hash_tree::stop()
{
	stop_thread = true;
}

bool hash_tree::read_block(const tree_info & Tree_Info, const boost::uint64_t & block_num, std::string & block)
{
	std::pair<boost::uint64_t, unsigned> info;
	if(block_info(block_num, Tree_Info.row, info)){
		//open file to read
		std::fstream fin((global::HASH_DIRECTORY + Tree_Info.root_hash).c_str(), std::ios::in | std::ios::binary);
		if(!fin.is_open()){
			LOGGER << "error opening file";
			return false;
		}else{
			//write file
			fin.seekg(info.first, std::ios::beg);
			fin.read(block_buff, info.second);
			block.clear();
			block.assign(block_buff, fin.gcount());
			return true;
		}
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

		//open file to write
		std::fstream fout((global::HASH_DIRECTORY + Tree_Info.root_hash).c_str(), std::ios::in | std::ios::out | std::ios::binary);
		if(!fout.is_open()){
			LOGGER << "error opening file";
			return false;
		}else{
			//write file
			fout.seekp(info.first, std::ios::beg);
			fout.write(block.data(), block.size());
			fout.close(); //must be closed to flush to file, check_contiguous can error if this is not here
			{
			boost::mutex::scoped_lock lock(*Tree_Info.Contiguous_mutex);
			Tree_Info.Contiguous->insert(std::make_pair(block_num, IP));
			}
			check_contiguous(Tree_Info);
			return true;
		}
	}else{
		LOGGER << "client requested invalid block number";
		DB_Blacklist.add(IP);
		return false;
	}
}
