#include "p2p_real.hpp"

p2p_real::p2p_real():
	DB(path::database())
{
	//setup directories
	#ifndef _WIN32
	//main directory always in current directory on windows
	boost::filesystem::create_directory(path::main());
	#endif
	boost::filesystem::create_directory(path::download());
	boost::filesystem::create_directory(path::download_unfinished());
	boost::filesystem::create_directory(path::share());
	boost::filesystem::create_directory(path::temp());

	//setup database tables
	database::init::run(path::database());

	//trigger initialization of singletons
	number_generator::singleton();
	share::singleton();
}

p2p_real::~p2p_real()
{
	number_generator::singleton().stop();
	share::singleton().stop();
}

bool p2p_real::current_download(const std::string & hash, download_status & status)
{
	return false;
}

void p2p_real::current_downloads(std::vector<download_status> & CD)
{

}

void p2p_real::current_uploads(std::vector<upload_status> & CU)
{

}

bool p2p_real::file_info(const std::string & hash, std::string & path, boost::uint64_t & tree_size, boost::uint64_t & file_size)
{
/*
	if(database::table::download::lookup_hash(hash, path, file_size, DB)){
		tree_size = hash_tree::tree_info::file_size_to_tree_size(file_size);
		return true;
	}else{
		return false;
	}
*/
	return false;
}

unsigned p2p_real::get_max_connections()
{
	return 666;
}

unsigned p2p_real::get_max_download_rate()
{
	return 666;
}

unsigned p2p_real::get_max_upload_rate()
{
	return 666;
}

void p2p_real::pause_download(const std::string & hash)
{
	//p2p_buffer::pause_download(hash);
}

unsigned p2p_real::prime_count()
{
	return number_generator::singleton().prime_count();
}

void p2p_real::reconnect_unfinished()
{

}

boost::uint64_t p2p_real::share_size()
{
	return share::singleton().share_size();
}

//DEBUG, this search needs to be asynchronous
void p2p_real::search(std::string search_term, std::vector<download_info> & Search_Info)
{
	/*
	Have function that gui thread can poll for results. As results are returned
	results stored internally to the p2p lib are removed. The function to get the
	results can return false when it has no more results to return for the search.
	*/

	LOGGER << "FUNCTION NEEDS TO BE MADE ASYNC";
	exit(1);
	database::table::search::run_search(search_term, Search_Info, DB);
}

void p2p_real::set_max_connections(const unsigned max_connections)
{

}

void p2p_real::set_max_download_rate(const unsigned max_download_rate)
{

}

void p2p_real::set_max_upload_rate(const unsigned max_upload_rate)
{

}

void p2p_real::start_download(const download_info & DI)
{

}

void p2p_real::remove_download(const std::string & hash)
{

}

unsigned p2p_real::download_rate()
{
	return 666;
}

unsigned p2p_real::upload_rate()
{
	return 666;
}
