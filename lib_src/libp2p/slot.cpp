#include "slot.hpp"

slot::slot(
	const file_info & FI,
	database::pool::proxy DB
):
	Hash_Tree(FI, DB),
	File(FI)
{
	boost::shared_ptr<std::vector<database::table::host::host_info> >
		H = database::table::host::lookup(Hash_Tree.hash, DB);
	if(H){
		host.insert(H->begin(), H->end());
	}
}

bool slot::complete()
{
	bool temp = true;
	if(!Hash_Tree.complete()){
		temp = false;
	}

/*
	if(!File.complete()){
		temp = false;
	}
*/
	return temp;
}

unsigned slot::download_speed()
{
	return 1024;
}

const boost::uint64_t & slot::file_size()
{
	return Hash_Tree.file_size;
}

const std::string & slot::hash()
{
	return Hash_Tree.hash;
}

void slot::merge_host(std::set<database::table::host::host_info> & host_in)
{
	boost::mutex::scoped_lock lock(host_mutex);
	host_in.insert(host.begin(), host.end());
}

std::string slot::name()
{
	int pos;
	if((pos = File.path.rfind('/')) != std::string::npos){
		return File.path.substr(pos+1);
	}else{
		return "";
	}
}

unsigned slot::percent_complete()
{
	return 69;
}

const boost::uint64_t & slot::tree_size()
{
	return Hash_Tree.tree_size;
}

unsigned slot::upload_speed()
{
	return 1024;
}
