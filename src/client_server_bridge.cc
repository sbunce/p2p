#include "client_server_bridge.h"

client_server_bridge * client_server_bridge::Client_Server_Bridge = NULL;
boost::mutex client_server_bridge::init_mutex;

client_server_bridge::client_server_bridge()
{

}


void client_server_bridge::download_block_receieved_priv(const std::string & hash, const boost::uint64_t & block_number)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::map<std::string, download_state>::iterator iter = Download.find(hash);
	assert(iter != Download.end());
	iter->second.add_block(block_number);
}

void client_server_bridge::finish_download_priv(const std::string & hash)
{
	boost::mutex::scoped_lock lock(Mutex);
	Download.erase(hash);
}

bool client_server_bridge::is_available_priv(const std::string & hash, const boost::uint64_t & block_number)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::map<std::string, download_state>::iterator iter = Download.find(hash);
	assert(iter != Download.end());
	return iter->second.is_available(block_number);
}

bool client_server_bridge::is_downloading_priv(const std::string & hash)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::map<std::string, download_state>::iterator iter = Download.find(hash);
	return iter != Download.end();
}

void client_server_bridge::start_download_priv(const std::string & hash)
{
	boost::mutex::scoped_lock lock(Mutex);
	Download.insert(std::make_pair(hash, download_state(DOWNLOAD_HASH_TREE)));
}

void client_server_bridge::transition_download_priv(const std::string & hash)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::map<std::string, download_state>::iterator iter = Download.find(hash);
	assert(iter != Download.end());
	iter->second.transition_download();
}
