#include "download_hash_tree.h"

download_hash_tree::download_hash_tree(
	const std::string & root_hash_in,
	const unsigned long & file_size,
	const std::string & file_name
):
	root_hash(root_hash_in)
{

}

bool download_hash_tree::complete()
{

}

const std::string & download_hash_tree::hash()
{
	return root_hash;
}

unsigned int download_hash_tree::max_response_size()
{

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

}

const uint64_t & download_hash_tree::total_size()
{
	return hash_tree_size;
}
