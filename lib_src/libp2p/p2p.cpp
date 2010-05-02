//custom
#include "p2p_impl.hpp"

//include
#include <boost/shared_ptr.hpp>
#include <p2p.hpp>

p2p::p2p():
	P2P_impl(new p2p_impl())
{

}

unsigned p2p::download_rate()
{
	return P2P_impl->download_rate();
}

unsigned p2p::get_max_connections()
{
	return P2P_impl->get_max_connections();
}

unsigned p2p::get_max_download_rate()
{
	return P2P_impl->get_max_download_rate();
}

unsigned p2p::get_max_upload_rate()
{
	return P2P_impl->get_max_upload_rate();
}

void p2p::remove_download(const std::string & hash)
{
	P2P_impl->remove_download(hash);
}

void p2p::set_max_connections(const unsigned connections)
{
	P2P_impl->set_max_connections(connections);
}

void p2p::set_max_download_rate(const unsigned rate)
{
	P2P_impl->set_max_download_rate(rate);
}

void p2p::set_max_upload_rate(const unsigned rate)
{
	P2P_impl->set_max_upload_rate(rate);
}

boost::uint64_t p2p::share_size_bytes()
{
	return P2P_impl->share_size_bytes();
}

boost::uint64_t p2p::share_size_files()
{
	return P2P_impl->share_size_files();
}

void p2p::start_download(const p2p::download & D)
{
	P2P_impl->start_download(D);
}

void p2p::test(const std::string & port, const std::string & program_directory)
{
	path::override_database_name("DB_"+port);
	path::override_program_directory(program_directory);
	db::init::create_all();
	db::table::prefs::set_port(port);
}

void p2p::transfers(std::vector<p2p::transfer> & T)
{
	P2P_impl->transfers(T);
}

unsigned p2p::upload_rate()
{
	return P2P_impl->upload_rate();
}
