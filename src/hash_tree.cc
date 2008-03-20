//boost
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

//std
#include <fstream>
#include <iostream>

#include "hash_tree.h"

hash_tree::hash_tree()
{
	//create hash directory if it doesn't already exist
	boost::filesystem::create_directory(global::HASH_DIRECTORY.c_str());

	SHA.init(global::HASH_TYPE);
	hash_length = SHA.hash_length();
	block_size = global::P_BLS_SIZE - 1;
	hash_count = 0;
}

bool hash_tree::check_hash(char parent[], char left_child[], char right_child[])
{
	SHA.init(sha::enuSHA1);
	SHA.update((const unsigned char *)left_child, SHA.hash_length());
	SHA.update((const unsigned char *)right_child, SHA.hash_length());
	SHA.end();

	if(memcmp(parent, SHA.raw_hash_no_null(), SHA.hash_length()) == 0){
		return true;
	}
	else{
		return false;
	}
}

bool hash_tree::check_hash_tree(std::string root_hash, unsigned long & hash_count, std::pair<unsigned long, unsigned long> & bad_hash)
{
	//reset the positions if checking a different tree
	if(check_tree_latest.empty() || check_tree_latest != root_hash){
		current_RRN = 0; //RRN of latest hash retrieved
		start_RRN = 0;   //RRN of the start of the current row
		end_RRN = 0;     //RRN of the end of the current row
	}

	std::fstream hash_fstream((global::HASH_DIRECTORY+root_hash).c_str(), std::fstream::in | std::fstream::binary);

	//a null hash represends the early termination of a row
	char end_of_row[hash_length];
	memset(end_of_row, '\0', hash_length);

	//buffers for a parent and two child nodes
	char hash[hash_length];
	char left_child[hash_length];
	char right_child[hash_length];

	bool early_termination = false;
	while(true){

		hash_fstream.seekg(current_RRN * hash_length);
		hash_fstream.read(hash, hash_length);

		//detect early termination of a row
		if(memcmp(hash, end_of_row, hash_length) == 0){
			early_termination = true;
			++current_RRN;
			continue;
		}

		//calculate start and end RRNs of the current row
		if(current_RRN == end_RRN + 1){
			unsigned long start_RRN_temp = start_RRN;
			start_RRN = end_RRN + 1;
			end_RRN = 2 * (end_RRN - start_RRN_temp) + current_RRN + 1;

			if(early_termination){
				end_RRN -= 2;
				early_termination = false;
			}
		}

		unsigned long first_child_RRN = 2 * (current_RRN - start_RRN) + end_RRN + 1;

		//stopping case, all hashes in the second row checked, bottom file hash row good
		if(first_child_RRN + 1 >= hash_count){
			hash_fstream.close();
			return false;
		}

		//read child nodes
		hash_fstream.seekg(first_child_RRN * hash_length);
		hash_fstream.read(left_child, hash_length);
		hash_fstream.read(right_child, hash_length);

		if(!check_hash(hash, left_child, right_child)){
			bad_hash.first = (2 * (current_RRN - start_RRN) + end_RRN + 1);
			bad_hash.second = (2 * (current_RRN - start_RRN) + end_RRN + 2);
			hash_fstream.close();
			return true;
		}

		++current_RRN;
	}

	hash_fstream.close();
}

std::string hash_tree::create_hash_tree(std::string file_name)
{
	//holds one file block
	char chunk[block_size];

	//null hash for comparison
	char empty[hash_length];
	memset(empty, '\0', hash_length);

	std::fstream fin(file_name.c_str(), std::ios::in | std::ios::binary);
	std::fstream scratch((global::HASH_DIRECTORY+"scratch").c_str(), std::ios::in
		| std::ios::out | std::ios::trunc | std::ios::binary);

	do{
		fin.read(chunk, block_size);
		++hash_count;
		SHA.init(sha::enuSHA1);
		SHA.update((const unsigned char *)chunk, fin.gcount());
		SHA.end();
		scratch.write(SHA.raw_hash_no_null(), hash_length);
	}while(fin.good());

	if(fin.bad() || !fin.eof()){
		global::debug_message(global::FATAL,__FILE__,__FUNCTION__,"error reading file");
	}

	//will contain the root hash in hex format
	std::string root_hash;

	//start recursive tree generating function
	create_hash_tree_recurse(scratch, 0, scratch.tellp(), root_hash);
	scratch.close();

	//clear the scratch file
	scratch.open((global::HASH_DIRECTORY+"scratch").c_str(), std::ios::trunc | std::ios::out);
	scratch.close();

	return root_hash;
}

void hash_tree::create_hash_tree_recurse(std::fstream & scratch, std::streampos row_start, std::streampos row_end, std::string & root_hash)
{
	//holds one hash
	char chunk[hash_length];

	//null hash for comparison
	char empty[hash_length];
	memset(empty, '\0', hash_length);

	//stopping case, if one hash passed in it's the root hash
	if(row_end - row_start == hash_length){
		return;
	}

	//if passed an odd amount of hashes append a NULL hash to inicate termination
	if(((row_end - row_start)/hash_length) % 2 != 0){
		scratch.seekp(row_end);
		scratch.write(empty, hash_length);
		row_end += hash_length;
		++hash_count;
	}

	//save position of last read and write with these, restore when switching
	std::streampos scratch_read, scratch_write;
	scratch_read = row_start;
	scratch_write = row_end;

	//loop through all hashes in the lower row and create the next highest row
	scratch.seekg(scratch_read);
	while(scratch_read != row_end){
		//each new upper hash requires two lower hashes

		//read hash 1
		scratch.seekg(scratch_read);
		scratch.read(chunk, hash_length);
		scratch_read = scratch.tellg();
		SHA.init(sha::enuSHA1);
		SHA.update((const unsigned char *)chunk, scratch.gcount());

		//read hash 2
		scratch.read(chunk, hash_length);
		scratch_read = scratch.tellg();
		SHA.update((const unsigned char *)chunk, scratch.gcount());

		SHA.end();
		++hash_count;

		//write resulting hash
		scratch.seekp(scratch_write);
		scratch.write(SHA.raw_hash_no_null(), hash_length);
		scratch_write = scratch.tellp();
	}

	//recurse
	create_hash_tree_recurse(scratch, row_end, scratch_write, root_hash);

	/*
	Writing of the result hash tree file is depth first, the root node will be
	added first.
	*/
	std::fstream fout;
	if(scratch_write - row_end == hash_length){
		//create the file with the root hash as name
		root_hash = SHA.string_hex_hash();
		fout.open((global::HASH_DIRECTORY+root_hash).c_str(), std::ios::out | std::ios::app | std::ios::binary);
		fout.write(SHA.raw_hash_no_null(), SHA.hash_length());
	}
	else{
		//this is not the recursive call with the root hash
		fout.open((global::HASH_DIRECTORY+root_hash).c_str(), std::ios::out | std::ios::app | std::ios::binary);
	}

	//write hashes that were passed to this function
	scratch.seekg(row_start);
	while(scratch.tellg() != row_end){
		scratch.read(chunk, hash_length);
		fout.write(chunk, hash_length);
	}

	fout.close();

	return;
}

unsigned long hash_tree::locate_start(std::string root_hash)
{
	//null hash used to determine if a row is terminating early
	char end_of_row[hash_length];
	memset(end_of_row, '\0', hash_length);

	std::fstream fin((global::HASH_DIRECTORY+root_hash).c_str(), std::fstream::in | std::fstream::binary);
	fin.seekg(0, std::ios::end);
	unsigned int max_possible_RRN = fin.tellg() / hash_length - 1;

	unsigned long current_RRN = 0;
	unsigned long start_RRN = 0;   //RRN of the start of the current row
	unsigned long end_RRN = 0;     //RRN of the end of the current row

	//holder for one hash
	char hash[hash_length];

	while(true){
		fin.seekg(current_RRN * hash_length);
		fin.read(hash, hash_length);

		if(memcmp(hash, end_of_row, hash_length) == 0){
			//row terminated early
			unsigned long start_RRN_temp = start_RRN;
			start_RRN = end_RRN + 1;
			end_RRN = 2 * (end_RRN - start_RRN_temp) + current_RRN;
			current_RRN = end_RRN;

			if(end_RRN == max_possible_RRN){
				fin.close();
				return start_RRN;
			}

			if(end_RRN > max_possible_RRN){
				global::debug_message(global::FATAL,__FILE__,__FUNCTION__,"corrupt hash tree");
			}
		}
		else{
			unsigned long start_RRN_temp = start_RRN;
			start_RRN = end_RRN + 1;
			end_RRN = 2 * (end_RRN - start_RRN_temp) + current_RRN + 2;
			current_RRN = end_RRN;

			if(end_RRN == max_possible_RRN){
				fin.close();
				return start_RRN;
			}

			if(end_RRN > max_possible_RRN){
				global::debug_message(global::FATAL,__FILE__,__FUNCTION__,"corrupt hash tree");
			}
		}
	}
}

void hash_tree::replace_hash(std::fstream & hash_fstream, const unsigned long & number, char hash[])
{
	hash_fstream.seekp(hash_length * number);
	hash_fstream.write(hash, hash_length);
}
