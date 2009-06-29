//custom
#include "p2p_real.hpp"

//include
#include <boost/shared_ptr.hpp>
#include <p2p/p2p.hpp>

p2p::p2p():
	P2P_Real(new p2p_real())
{

}

bool p2p::current_download(const std::string & hash, download_status & status)
{
	return P2P_Real->current_download(hash, status);
}

void p2p::current_downloads(std::vector<download_status> & CD)
{
	P2P_Real->current_downloads(CD);
}

void p2p::current_uploads(std::vector<upload_status> & CU)
{
	P2P_Real->current_uploads(CU);
}

unsigned p2p::download_rate()
{
	return P2P_Real->download_rate();
}

bool p2p::file_info(const std::string & hash, std::string & path,
	boost::uint64_t & tree_size, boost::uint64_t & file_size)
{
	return P2P_Real->file_info(hash, path, tree_size, file_size);
}

unsigned p2p::get_max_connections()
{
	return P2P_Real->get_max_connections();
}

unsigned p2p::get_max_download_rate()
{
	return P2P_Real->get_max_download_rate();
}

unsigned p2p::get_max_upload_rate()
{
	return P2P_Real->get_max_upload_rate();
}

void p2p::pause_download(const std::string & hash)
{
	P2P_Real->pause_download(hash);
}

unsigned p2p::prime_count()
{
	return P2P_Real->prime_count();
}

void p2p::remove_download(const std::string & hash)
{
	P2P_Real->remove_download(hash);
}

void p2p::search(std::string search_term, std::vector<download_info> & Search_Info)
{
	P2P_Real->search(search_term, Search_Info);
}

void p2p::set_max_connections(const unsigned max_connections)
{
	P2P_Real->set_max_connections(max_connections);
}

void p2p::set_max_download_rate(const unsigned max_download_rate)
{
	P2P_Real->set_max_download_rate(max_download_rate);
}

void p2p::set_max_upload_rate(const unsigned max_upload_rate)
{
	P2P_Real->set_max_upload_rate(max_upload_rate);
}

boost::uint64_t p2p::share_size()
{
	return P2P_Real->share_size();
}

void p2p::start_download(const download_info & DI)
{
	P2P_Real->start_download(DI);
}

unsigned p2p::upload_rate()
{
	return P2P_Real->upload_rate();
}
