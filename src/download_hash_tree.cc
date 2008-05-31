#include "download_hash_tree.h"

download_hash_tree::download_hash_tree(const std::string & root_hash_in, const unsigned long & file_size,
const std::string & file_name, volatile bool * download_complete_flag_in)
:
root_hash(root_hash_in),
download_complete_flag(download_complete_flag_in)
{
	hash_name = file_name + "HASH";
	hash_tree_count = hash_tree::file_size_to_hash_tree_count(file_size);
	hash_tree_size = hash_tree_count * 20;

	if(Hash_Tree.check_exists(root_hash)){
		//the hash tree already exists, see if it's complete and correct
		std::pair<unsigned long, unsigned long> bad_hash;
		if(Hash_Tree.check_hash_tree(root_hash, hash_tree_count, bad_hash)){
			//hash tree not complete or correct, resume building it from first incomplete/incorrect part
			Request_Gen.init(bad_hash.first, hash_tree_count, global::TIMEOUT);
		}
		else{
			//hash tree already complete and correct
			*download_complete_flag = true;
		}
	}
	else{
		//hash tree doesn't yet exist, start downloading it
		Request_Gen.init(0, hash_tree_count, global::TIMEOUT);
	}
}

bool download_hash_tree::complete()
{

//DEBUG, bad idea to do a hash check here, it wastes compute time and nothing done with results
	std::pair<unsigned long, unsigned long> bad_hash;
	return !Hash_Tree.check_hash_tree(root_hash, hash_tree_count, bad_hash);
}

const std::string & download_hash_tree::hash()
{
	return root_hash;
}

unsigned int download_hash_tree::max_response_size()
{
	//figure this out
}

const std::string & download_hash_tree::name()
{
	return hash_name;
}

unsigned int download_hash_tree::percent_complete()
{
	return (unsigned int)((latest_hash_received/(float)hash_tree_count)*100);
}

bool download_hash_tree::request(const int & socket, std::string & request, std::vector<std::pair<char, int> > & expected)
{

}

void download_hash_tree::response(const int & socket, std::string block)
{

}

void download_hash_tree::stop()
{
	*download_complete_flag = true;
}

const unsigned long & download_hash_tree::total_size()
{
	return hash_tree_size;
}
