#include "hash_tree.h"

boost::mutex hash_tree::generate_mutex;

hash_tree::hash_tree():
	stop_thread(false)
{
	//create hash directory if it doesn't already exist
	boost::filesystem::create_directory(global::HASH_DIRECTORY.c_str());
}

bool hash_tree::check(const tree_info & Tree_Info, boost::uint64_t & bad_block)
{
	//keeps track of current block so bad_block can be set if bad block detected
	boost::uint64_t current_block = 0;

	//used to keep track of how full the block_buff is
	int block_buff_size = 0;

	if(Tree_Info.block_count == 0){
		//only one hash, it's the root hash and the file hash
		return true;
	}

	//reserve space for speed up
	SHA.reserve(global::HASH_BLOCK_SIZE * sha::HASH_SIZE);

	std::fstream fin((global::HASH_DIRECTORY + Tree_Info.root_hash).c_str(), std::ios::in | std::ios::binary);
	if(!fin.is_open()){
		//hash tree doesn't exist
		bad_block = 0;
		return false;
	}

	//check first row with root hash
	std::string root_hash_bin = convert::hex_to_binary(Tree_Info.root_hash);
	if(Tree_Info.row[0] * sha::HASH_SIZE < sha::MIN_DATA_SIZE){
		//not enough hashes to meet sha::MIN_DATA_SIZE, more documentation on this in generate_hash_tree()
		for(boost::uint64_t x = 0; x * sha::HASH_SIZE < sha::MIN_DATA_SIZE; ++x){
			fin.seekg((x % Tree_Info.row[0]) * sha::HASH_SIZE , std::ios::beg);
			fin.read(block_buff + block_buff_size, sha::HASH_SIZE);
			if(!fin.good()){
				bad_block = 0;
				logger::debug(LOGGER_P1,"bad block ",bad_block," in ",Tree_Info.root_hash);
				return false;
			}
			block_buff_size += sha::HASH_SIZE;
		}
	}else{
		fin.read(block_buff, Tree_Info.row[0] * sha::HASH_SIZE);
		if(!fin.good()){
			bad_block = 0;
			logger::debug(LOGGER_P1,"bad block ",bad_block," in ",Tree_Info.root_hash);
			return false;
		}
		block_buff_size = Tree_Info.row[0] * sha::HASH_SIZE;
	}
	SHA.init();
	SHA.load(block_buff, block_buff_size);
	SHA.end();
	if(strncmp(root_hash_bin.data(), SHA.raw_hash(), sha::HASH_SIZE) != 0){
		bad_block = current_block;
		logger::debug(LOGGER_P1,"bad block ",bad_block," in ",Tree_Info.root_hash);
		return false;
	}

	//check lower rows
	boost::uint64_t row_start = 0, row_end = 0; //hash RRNs
	boost::uint64_t parent = 0;                 //RRN of parent hash
	current_block = 1;
	for(int x=1; x<Tree_Info.row.size(); ++x){
		row_start = row_start + Tree_Info.row[x-1];
		row_end = row_start + Tree_Info.row[x];
		for(int y=0; y<Tree_Info.row[x]; y+=global::HASH_BLOCK_SIZE){
			boost::uint64_t current = row_start + y;
			block_buff_size = 0;
			if(row_end - current < global::HASH_BLOCK_SIZE){
				if((row_end - current) * sha::HASH_SIZE < sha::MIN_DATA_SIZE){
					/*
					Not enough hashes left in row to meet sha::MIN_DATA_SIZE. Loop back
					to beginning or row to get enough hashes.

					Further documentation about this in generate_hash_tree().
					*/
					boost::uint64_t row_RRN_length = row_end - row_start; //# of hashes in row
					boost::uint64_t offset_RRN = current - row_start;     //offset from start of row

					/*
					Loop over row, starting at current position, until the amount of
					hashes gotten is >= sha::MIN_DATA_SIZE.
					*/
					for(boost::uint64_t tmp = offset_RRN; (tmp - offset_RRN) * sha::HASH_SIZE < sha::MIN_DATA_SIZE; ++tmp){
						fin.seekg((row_start + tmp % row_RRN_length) * sha::HASH_SIZE , std::ios::beg);
						fin.read(block_buff + block_buff_size, sha::HASH_SIZE);
						if(!fin.good()){
							bad_block = current_block;
							return false;
						}
						block_buff_size += sha::HASH_SIZE;
					}
				}else{
					/*
					Enough hashes to meet sha::MIN_DATA_SIZE. Read the remainder of
					the hashes.
					*/
					fin.seekg(current * sha::HASH_SIZE, std::ios::beg);
					fin.read(block_buff, (row_end - current) * sha::HASH_SIZE);
					if(!fin.good()){
						bad_block = current_block;
						return false;
					}
					block_buff_size = (row_end - current) * sha::HASH_SIZE;
				}
			}else{
				//enough hashes for full hash block
				fin.seekg(current * sha::HASH_SIZE, std::ios::beg);
				fin.read(block_buff, global::HASH_BLOCK_SIZE * sha::HASH_SIZE);
				if(!fin.good()){
					bad_block = current_block;
					return false;
				}
				block_buff_size = global::HASH_BLOCK_SIZE * sha::HASH_SIZE;
			}

			//create hash for children
			SHA.init();
			SHA.load(block_buff, block_buff_size);
			SHA.end();

			//read parent hash
			fin.seekg(parent * sha::HASH_SIZE, std::ios::beg);
			fin.read(block_buff, sha::HASH_SIZE);
			if(!fin.good()){
				logger::debug(LOGGER_P1,"error reading hash tree ",Tree_Info.root_hash);
				exit(1);
			}

			//validate
			if(strncmp(block_buff, SHA.raw_hash(), sha::HASH_SIZE) != 0){
				bad_block = current_block;
				logger::debug(LOGGER_P1,"bad block ",bad_block," in ",Tree_Info.root_hash);
				return false;
			}
			++current_block;
			++parent;
		}
		parent = row_start;
	}

	return true;
}

bool hash_tree::create(const std::string & file_path, std::string & root_hash)
{
	boost::mutex::scoped_lock lock(generate_mutex);

	//reserve space for speed up
	SHA.reserve(global::HASH_BLOCK_SIZE * sha::HASH_SIZE);

	//open file to hash, and create scratch file in which to build hash tree
	std::fstream fin(file_path.c_str(), std::ios::in | std::ios::binary);
	if(!fin.good()){
		logger::debug(LOGGER_P1,"error opening file ", file_path);
		return false;
	}
	std::fstream scratch((global::HASH_DIRECTORY+"upside_down").c_str(), std::ios::in
		| std::ios::out | std::ios::trunc | std::ios::binary);
	if(!scratch.good()){
		logger::debug(LOGGER_P1,"error opening scratch file");
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
			logger::debug(LOGGER_P1,"error writing to scratch file");
			return false;
		}
		++blocks_read;

		if(stop_thread){
			return false;
		}
	}

	if(fin.bad() || !fin.eof()){
		logger::debug(LOGGER_P1,"error reading file \"",file_path,"\"");
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
	unsigned int offset = 0;

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
			if((end_RRN - scratch_read_RRN) * sha::HASH_SIZE < sha::MIN_DATA_SIZE){
				/*
				Not enough hashes to meet sha::MIN_DATA_SIZE. In this case hash past
				the end modulo row size. In other words if there isn't enough hashes
				remaining in the row to meet the sha::MIN_DATA_SIZE then include
				hashes at beginning of row until sha::MIN_DATA_SIZE is reached.

				Example: Let each number represent a hash. Assume we need a minimum of
				         4 hashes to meet sha::MIN_DATA_SIZE.
				start:  A B
				hashed: A B A B

				Example 2:
				start:  A B C
				hashed: A B C A
				*/
				boost::uint64_t row_RRN_length = end_RRN - start_RRN;      //# of hashes in row
				boost::uint64_t offset_RRN = scratch_read_RRN - start_RRN; //offset from start of row

				/*
				Loop over row, starting at current position, until the amount of
				hashes gotten is >= sha::MIN_DATA_SIZE.
				*/
				for(boost::uint64_t x = offset_RRN; (x - offset_RRN) * sha::HASH_SIZE < sha::MIN_DATA_SIZE; ++x){
					scratch.seekg((start_RRN + x % row_RRN_length) * sha::HASH_SIZE , std::ios::beg);
					scratch.read(block_buff + block_buff_size, sha::HASH_SIZE);
					if(!scratch.good()){
						logger::debug(LOGGER_P1,"error reading scratch file");
						return false;
					}
					block_buff_size += sha::HASH_SIZE;
				}
			}else{
				/*
				Enough hashes to meet sha::MIN_DATA_SIZE. Read the remainder of
				the hashes.
				*/
				scratch.seekg(scratch_read_RRN * sha::HASH_SIZE, std::ios::beg);
				scratch.read(block_buff, (end_RRN - scratch_read_RRN) * sha::HASH_SIZE);
				if(!scratch.good()){
					logger::debug(LOGGER_P1,"error reading scratch file");
					return false;
				}
				block_buff_size = (end_RRN - scratch_read_RRN) * sha::HASH_SIZE;
			}
		}else{
			//enough hashes for full hash block
			scratch.seekg(scratch_read_RRN * sha::HASH_SIZE, std::ios::beg);
			scratch.read(block_buff, global::HASH_BLOCK_SIZE * sha::HASH_SIZE);
			if(!scratch.good()){
				logger::debug(LOGGER_P1,"error reading scratch file");
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
			logger::debug(LOGGER_P1,"error writing scratch file");
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
			logger::debug(LOGGER_P1,"error opening rightside_up file");
			return false;
		}
	}else{
		//this is not the recursive call with the root hash
		fout.open((global::HASH_DIRECTORY+"rightside_up").c_str(), std::ios::out | std::ios::app | std::ios::binary);
		if(!scratch.good()){
			logger::debug(LOGGER_P1,"error opening rightside up file");
			return false;
		}
	}

	//write hashes that were passed to this function
	for(int x=start_RRN; x<end_RRN; ++x){
		scratch.seekg(x * sha::HASH_SIZE, std::ios::beg);
		scratch.read(block_buff, sha::HASH_SIZE);
		if(!scratch.good()){
			logger::debug(LOGGER_P1,"error reading scratch file");
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
		logger::debug(LOGGER_P1,"error reading hash tree ",Tree_Info.root_hash);
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
	std::pair<boost::uint64_t, boost::uint64_t> info;
	if(block_info(block_num, Tree_Info.row, info)){
		//open file to read
		std::fstream fin((global::HASH_DIRECTORY + Tree_Info.root_hash).c_str(), std::ios::in | std::ios::binary);
		if(!fin.is_open()){
			logger::debug(LOGGER_P1,"error opening file");
			return false;
		}else{
			//write file
			fin.seekg(info.first, std::ios::beg);
			fin.read(block_buff, info.second);
			block.clear();
			block.assign(block_buff, fin.gcount());
		}
		return true;
	}else{
		logger::debug(LOGGER_P1,"invalid block number, programming error");
		exit(1);
	}
}

bool hash_tree::write_block(const tree_info & Tree_Info, const boost::uint64_t & block_num, const std::string & block)
{
	std::pair<boost::uint64_t, boost::uint64_t> info;
	if(block_info(block_num, Tree_Info.row, info)){
		if(info.second != block.size()){
			//incorrect block size
			logger::debug(LOGGER_P1,"incorrect block size, programming error");
			exit(1);
		}

		//open file to write
		std::fstream fout((global::HASH_DIRECTORY + Tree_Info.root_hash).c_str(), std::ios::in | std::ios::out | std::ios::binary);
		if(!fout.is_open()){
			logger::debug(LOGGER_P1,"error opening file");
			return false;
		}else{
			//write file
			fout.seekp(info.first, std::ios::beg);
			fout.write(block.data(), block.size());
		}
		return true;
	}else{
		logger::debug(LOGGER_P1,"invalid block number, programming error");
		exit(1);
	}
}
