#include "p2p.hpp"

p2p::p2p():
	DB(path::database())
{
	//setup directories
	boost::filesystem::create_directory(path::main());
	boost::filesystem::create_directory(path::download());
	boost::filesystem::create_directory(path::download_unfinished());
	boost::filesystem::create_directory(path::share());

	//setup database tables
	database::init::run(path::database());

	//trigger initialization of singletons
	number_generator::singleton();
}

p2p::~p2p()
{
	number_generator::singleton().stop();
}

bool p2p::current_download(const std::string & hash, download_status & status)
{

}

void p2p::current_downloads(std::vector<download_status> & status)
{

}

void p2p::current_uploads(std::vector<upload_status> & info)
{

}

bool p2p::file_info(const std::string & hash, std::string & path, boost::uint64_t & tree_size, boost::uint64_t & file_size)
{
	if(database::table::download::lookup_hash(hash, path, file_size, DB)){
		tree_size = hash_tree::tree_info::file_size_to_tree_size(file_size);
		return true;
	}else{
		return false;
	}
}

unsigned p2p::get_max_connections()
{
	return 666;
}

unsigned p2p::get_max_download_rate()
{
	return 666;
}

unsigned p2p::get_max_upload_rate()
{
	return 666;
}

bool p2p::is_indexing()
{
	return share_index::singleton().is_indexing();
}

void p2p::pause_download(const std::string & hash)
{
	//p2p_buffer::pause_download(hash);
}

unsigned p2p::prime_count()
{
	return number_generator::singleton().prime_count();
}

void p2p::reconnect_unfinished()
{

}

void p2p::search(std::string search_word, std::vector<download_info> & Search_Info)
{
	database::table::search::run_search(search_word, Search_Info, DB);
}

void p2p::set_max_connections(const unsigned & max_connections_in)
{

}

void p2p::set_max_download_rate(unsigned download_rate)
{

}

void p2p::set_max_upload_rate(unsigned upload_rate)
{

}

void p2p::start_download(const download_info & info)
{

}

void p2p::remove_download(const std::string & hash)
{

}

unsigned p2p::download_rate()
{
	return 666;
}

unsigned p2p::upload_rate()
{
	return 666;
}
