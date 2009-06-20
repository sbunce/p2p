#ifndef H_DOWNLOAD
#define H_DOWNLOAD

//API
#include <p2p/download_info.hpp>
#include <p2p/download_status.hpp>

//custom
#include "hash_tree.hpp"
#include "path.hpp"
#include "request_generator.hpp"

//inclucde
#include <speed_calculator.hpp>

//std
#include <map>

class download : private boost::noncopyable
{
public:
	download(const download_info & info)
	{

	}

	const std::string & get_download_hash()
	{
		//return Tree_Info.get_root_hash();
//DEBUG bogus
		return name;
	}

	void get_download_status(download_status & status)
	{
		//status.hash = Tree_Info.get_root_hash();
		status.name = name;
		status.size = 0;
		status.percent_complete = 0;
		status.servers.clear();
	}

private:
	//name of downloading file
	std::string name;

	//location of downloading file
	std::string file_path;

/*
	hash_tree Hash_Tree;
	request_generator Request_Generator;
	hash_tree::tree_info Tree_Info;
*/
};
#endif
