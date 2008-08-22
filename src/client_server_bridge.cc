#include "client_server_bridge.h"

client_server_bridge * client_server_bridge::Client_Server_Bridge = NULL;
boost::mutex client_server_bridge::init_mutex;

client_server_bridge::client_server_bridge()
{

}

bool client_server_bridge::file_block_available_priv(const std::string & hash, const boost::uint64_t & block_number)
{

}

void client_server_bridge::file_block_receieved_priv(const std::string & hash, const boost::uint64_t & block_number)
{

}

void client_server_bridge::finish_file_priv(const std::string & hash)
{

}

void client_server_bridge::start_file_priv(const std::string & hash)
{

}
