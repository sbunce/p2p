//custom
#include "p2p_real.hpp"

//include
#include <boost/shared_ptr.hpp>
#include <p2p/p2p.hpp>

p2p::p2p():
	P2P_Real(new p2p_real())
{

}

void p2p::downloads(std::vector<download_status> & CD)
{
	P2P_Real->downloads(CD);
}

unsigned p2p::download_rate()
{
	return P2P_Real->download_rate();
}

unsigned p2p::max_connections()
{
	return P2P_Real->max_connections();
}

void p2p::max_connections(const unsigned connections)
{
	P2P_Real->max_connections(connections);
}

unsigned p2p::max_download_rate()
{
	return P2P_Real->max_download_rate();
}

void p2p::max_download_rate(const unsigned rate)
{
	P2P_Real->max_download_rate(rate);
}

unsigned p2p::max_upload_rate()
{
	return P2P_Real->max_upload_rate();
}

void p2p::max_upload_rate(const unsigned rate)
{
	P2P_Real->max_upload_rate(rate);
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

boost::uint64_t p2p::share_size_bytes()
{
	return P2P_Real->share_size_bytes();
}

boost::uint64_t p2p::share_size_files()
{
	return P2P_Real->share_size_files();
}

void p2p::start_download(const download_info & DI)
{
	P2P_Real->start_download(DI);
}

unsigned p2p::upload_rate()
{
	return P2P_Real->upload_rate();
}

void p2p::uploads(std::vector<upload_status> & CU)
{
	P2P_Real->uploads(CU);
}
