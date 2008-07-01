#include "hash_tree.h"

hash_tree::hash_tree(
	):
	stop_thread(false),
	SHA(global::FILE_BLOCK_SIZE)
{
	//create hash directory if it doesn't already exist
	boost::filesystem::create_directory(global::HASH_DIRECTORY.c_str());
}

bool hash_tree::check_block(const std::string & root_hex_hash, const uint64_t & block_number, const char * const block, const int & block_length)
{
	boost::mutex::scoped_lock lock(Mutex);
	if(root_hex_hash != file_hash_root_hex_hash){
		file_hash_root_hex_hash = root_hex_hash;
		file_hash_start_RRN = locate_start(root_hex_hash);
	}
	std::fstream fin((global::HASH_DIRECTORY+root_hex_hash).c_str(), std::fstream::in);
	fin.seekg(sha::HASH_LENGTH * file_hash_start_RRN + block_number * sha::HASH_LENGTH);
	fin.read(file_hash_buffer, sha::HASH_LENGTH);
	SHA.init();
	SHA.load(block, block_length);
	SHA.end();
	return strncmp(file_hash_buffer, SHA.raw_hash(), sha::HASH_LENGTH) == 0;
}

bool hash_tree::check_hash(char * parent, char * left_child, char * right_child)
{
	SHA.init();
	SHA.load(left_child, sha::HASH_LENGTH);
	SHA.load(right_child, sha::HASH_LENGTH);
	SHA.end();
	if(memcmp(parent, SHA.raw_hash(), sha::HASH_LENGTH) == 0){
		return true;
	}else{
		return false;
	}
}

bool hash_tree::check_hash_tree(const std::string & root_hash, const uint64_t & hash_count, std::pair<uint64_t, uint64_t> & bad_hash)
{
	boost::mutex::scoped_lock lock(Mutex);
	//reset the positions if checking a different tree
	if(check_tree_latest.empty() || check_tree_latest != root_hash){
		current_RRN = 0; //RRN of latest hash retrieved
		start_RRN = 0;   //RRN of the start of the current row
		end_RRN = 0;     //RRN of the end of the current row
	}
	std::fstream hash_fstream((global::HASH_DIRECTORY+root_hash).c_str(), std::fstream::in);
	if(!hash_fstream.is_open()){
		//hash tree does not yet exist
		bad_hash.first = 0;
		bad_hash.second = 1;
		return true;
	}

	//a null hash represends the early termination of a row
	char end_of_row[sha::HASH_LENGTH];
	memset(end_of_row, '\0', sha::HASH_LENGTH);

	//buffers for a parent and two child nodes
	char hash[sha::HASH_LENGTH];
	char left_child[sha::HASH_LENGTH];
	char right_child[sha::HASH_LENGTH];

	bool early_termination = false;
	while(true){
		hash_fstream.seekg(current_RRN * sha::HASH_LENGTH, std::ios::beg);
		hash_fstream.read(hash, sha::HASH_LENGTH);
		if(hash_fstream.eof()){
			//incomplete/missing parent hash
			bad_hash.first = current_RRN;
			bad_hash.second = current_RRN + 1;
			return true;
		}
		if(memcmp(hash, end_of_row, sha::HASH_LENGTH) == 0){
			//early termination of row
			early_termination = true;
			++current_RRN;
			continue;
		}
		if(current_RRN == end_RRN + 1){
			//calculate start and end RRNs of the current row
			uint64_t start_RRN_temp = start_RRN;
			start_RRN = end_RRN + 1;
			end_RRN = 2 * (end_RRN - start_RRN_temp) + current_RRN + 1;
			if(early_termination){
				end_RRN -= 2;
				early_termination = false;
			}
		}
		uint64_t first_child_RRN = 2 * (current_RRN - start_RRN) + end_RRN + 1;
		if(first_child_RRN + 1 > hash_count){
			//last hash checked, stop
			return false;
		}

		//read child nodes
		hash_fstream.seekg(first_child_RRN * sha::HASH_LENGTH, std::ios::beg);
		hash_fstream.read(left_child, sha::HASH_LENGTH);

		if(hash_fstream.eof()){
			//incomplete/missing child 1
			bad_hash.first = first_child_RRN;
			bad_hash.second = first_child_RRN + 1;
			return true;
		}

		hash_fstream.read(right_child, sha::HASH_LENGTH);

		if(hash_fstream.eof()){
			//incomplete/missing child
			bad_hash.first = first_child_RRN + 1;
			bad_hash.second = first_child_RRN + 2;
			return true;
		}

		if(!check_hash(hash, left_child, right_child)){
			//bad child hash
			bad_hash.first = first_child_RRN;
			bad_hash.second = first_child_RRN + 1;
			return true;
		}
		++current_RRN;
	}
}

bool hash_tree::create_hash_tree(std::string file_name, std::string & root_hash)
{
	char block_buff[global::FILE_BLOCK_SIZE];

	std::fstream fin(file_name.c_str(), std::ios::in | std::ios::binary);
	std::fstream scratch((global::HASH_DIRECTORY+"scratch").c_str(), std::ios::in
		| std::ios::out | std::ios::trunc | std::ios::binary);

	uint64_t blocks_read = 0;
	while(true){
		fin.read(block_buff, global::FILE_BLOCK_SIZE);
		if(fin.gcount() == 0){
			break;
		}

		SHA.init();
		SHA.load(block_buff, fin.gcount());
		SHA.end();
		scratch.write(SHA.raw_hash(), sha::HASH_LENGTH);
		++blocks_read;

		if(stop_thread){
			return false;
		}
	}

	if(fin.bad() || !fin.eof()){
		logger::debug(LOGGER_P1,"error reading file");
		exit(1);
	}

	//base case, the file size is less than one block
	if(blocks_read == 1){
		root_hash = SHA.hex_hash();
		std::fstream fout((global::HASH_DIRECTORY+root_hash).c_str(), std::ios::out | std::ios::app | std::ios::binary);
		fout.write(SHA.raw_hash(), sha::HASH_LENGTH);

		//clear the scratch file
		scratch.close();
		scratch.open((global::HASH_DIRECTORY+"scratch").c_str(), std::ios::trunc | std::ios::out);
		scratch.close();
		return true;
	}

	//start recursive tree generating function
	create_hash_tree_recurse(scratch, 0, scratch.tellp(), root_hash);
	scratch.close();

	//clear the scratch file
	scratch.open((global::HASH_DIRECTORY+"scratch").c_str(), std::ios::trunc | std::ios::out);
	scratch.close();

	return !root_hash.empty();
}

void hash_tree::create_hash_tree_recurse(std::fstream & scratch, std::streampos row_start, std::streampos row_end, std::string & root_hash)
{
	char hash_buff[sha::HASH_LENGTH];
	char empty[sha::HASH_LENGTH];
	memset(empty, '\0', sha::HASH_LENGTH);

	//stopping case, if one hash passed in it's the root hash
	if(row_end - row_start == sha::HASH_LENGTH){
		return;
	}

	//if passed an odd amount of hashes append a NULL hash to inicate termination
	if(((row_end - row_start)/sha::HASH_LENGTH) % 2 != 0){
		scratch.seekp(row_end);
		scratch.write(empty, sha::HASH_LENGTH);
		row_end += sha::HASH_LENGTH;
	}

	//save position of last read and write with these, restore when switching
	std::streampos scratch_read, scratch_write;
	scratch_read = row_start;
	scratch_write = row_end;

	//loop through all hashes in the lower row and create the next highest row
	scratch.seekg(scratch_read, std::ios::beg);
	while(scratch_read != row_end){
		if(stop_thread){
			return;
		}

		//each new upper hash requires two lower hashes
		scratch.seekg(scratch_read, std::ios::beg);
		scratch.read(hash_buff, sha::HASH_LENGTH);
		scratch_read = scratch.tellg();
		SHA.init();
		SHA.load(hash_buff, scratch.gcount());
		scratch.read(hash_buff, sha::HASH_LENGTH);
		scratch_read = scratch.tellg();
		SHA.load(hash_buff, scratch.gcount());
		SHA.end();

		//write resulting hash
		scratch.seekp(scratch_write, std::ios::beg);
		scratch.write(SHA.raw_hash(), sha::HASH_LENGTH);
		scratch_write = scratch.tellp();
	}

	//recurse
	create_hash_tree_recurse(scratch, row_end, scratch_write, root_hash);

	/*
	Writing of the result hash tree file is depth first, the root node will be
	added first.
	*/
	std::fstream fout;
	if(scratch_write - row_end == sha::HASH_LENGTH){
		root_hash = SHA.hex_hash();

		//do nothing if hash tree file already created
		std::fstream fin((global::HASH_DIRECTORY+root_hash).c_str(), std::ios::in);
		if(fin.is_open()){
			return;
		}

		//create the file with the root hash as name
		fout.open((global::HASH_DIRECTORY+root_hash).c_str(), std::ios::out | std::ios::app | std::ios::binary);
		fout.write(SHA.raw_hash(), sha::HASH_LENGTH);
	}else{
		//this is not the recursive call with the root hash
		fout.open((global::HASH_DIRECTORY+root_hash).c_str(), std::ios::out | std::ios::app | std::ios::binary);
	}

	//write hashes that were passed to this function
	scratch.seekg(row_start);
	while(scratch.tellg() != row_end){
		scratch.read(hash_buff, sha::HASH_LENGTH);
		fout.write(hash_buff, sha::HASH_LENGTH);
	}
	fout.close();
}

uint64_t hash_tree::locate_start(const std::string & root_hex_hash)
{
	//null hash used to determine if a row is terminating early
	char end_of_row[sha::HASH_LENGTH];
	memset(end_of_row, '\0', sha::HASH_LENGTH);

	std::fstream fin((global::HASH_DIRECTORY+root_hex_hash).c_str(), std::fstream::in);
	fin.seekg(0, std::ios::end);
	uint64_t max_possible_RRN = fin.tellg() / sha::HASH_LENGTH - 1;

	uint64_t current_RRN = 0;
	uint64_t start_RRN = 0;   //RRN of the start of the current row
	uint64_t end_RRN = 0;     //RRN of the end of the current row

	//holder for one hash
	char hash[sha::HASH_LENGTH];

	while(true){
		fin.seekg(current_RRN * sha::HASH_LENGTH);
		fin.read(hash, sha::HASH_LENGTH);
		if(memcmp(hash, end_of_row, sha::HASH_LENGTH) == 0){
			//row terminated early
			uint64_t start_RRN_temp = start_RRN;
			start_RRN = end_RRN + 1;
			end_RRN = 2 * (end_RRN - start_RRN_temp) + current_RRN;
			current_RRN = end_RRN;
			if(end_RRN == max_possible_RRN){
				fin.close();
				return start_RRN;
			}
			if(end_RRN > max_possible_RRN){
				logger::debug(LOGGER_P1,"corrupt hash tree ",root_hex_hash);
				exit(1);
			}
		}else{
			uint64_t start_RRN_temp = start_RRN;
			start_RRN = end_RRN + 1;
			end_RRN = 2 * (end_RRN - start_RRN_temp) + current_RRN + 2;
			current_RRN = end_RRN;
			if(end_RRN == max_possible_RRN){
				fin.close();
				return start_RRN;
			}
			if(end_RRN > max_possible_RRN){
				logger::debug(LOGGER_P1,"corrupt hash tree ",root_hex_hash);
				exit(1);
			}
		}
	}
}

void hash_tree::write_hash(const std::string & root_hex_hash, const uint64_t & number, const std::string & hash_block)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::fstream fout((global::HASH_DIRECTORY+root_hex_hash).c_str(), std::ios::in | std::ios::out | std::ios::binary);
	if(fout.is_open()){
		fout.seekp(sha::HASH_LENGTH * number, std::ios::beg);
		fout.write(hash_block.c_str(), hash_block.length());
	}else{
		logger::debug(LOGGER_P1,"error opening file ",global::HASH_DIRECTORY+root_hex_hash);
	}
}

void hash_tree::stop()
{
	stop_thread = true;
}
