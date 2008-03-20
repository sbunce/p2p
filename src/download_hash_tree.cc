#include "download_hash_tree.h"

download_hash_tree::download_hash_tree(std::string root_hash_in, bool * download_complete_flag_in)
{
	root_hash = root_hash_in;
	download_complete_flag = download_complete_flag_in;
}

bool download_hash_tree::complete()
{

}

unsigned int download_hash_tree::bytes_expected()
{

}

const std::string & download_hash_tree::hash()
{

}

const std::string & download_hash_tree::name()
{

}

unsigned int download_hash_tree::percent_complete()
{

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

const unsigned long & download_hash_tree::total_size()
{

}
