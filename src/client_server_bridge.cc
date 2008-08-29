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
	if(iter == Download.end()){
		//file no longer downloading, block is available
		return true;
	}else{
		//file downloading, check availability
		return iter->second.is_available(block_number);
	}
}

client_server_bridge::download_mode client_server_bridge::is_downloading_priv(const std::string & hash)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::map<std::string, download_state>::iterator iter = Download.find(hash);
	if(iter == Download.end()){
		return NOT_DOWNLOADING;
	}else{
		return iter->second.DM;
	}
}

bool client_server_bridge::is_downloading_hash_priv(const std::string & hash, std::string & path)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::map<std::string, download_state>::iterator iter = Download.find(hash);
	if(iter == Download.end()){
		return false;
	}else{
		path = global::HASH_DIRECTORY+hash;
		return true;
	}
}

bool client_server_bridge::is_downloading_file_priv(const std::string & hash, std::string & path)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::map<std::string, download_state>::iterator iter = Download.find(hash);
	if(iter == Download.end()){
		return false;
	}else{
		DB_Download.lookup_hash(hash, path);
		return true;
	}
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
