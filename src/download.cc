#include "download.h"

download::download()
{

}

download::~download()
{

}

const std::string download::hash()
{
	//if download visible this should be defined in derived
	return "FUNCTION NOT SET IN DERIVED";
}

void download::IP_list(std::vector<std::string> & IP)
{
	std::map<int, server_info>::iterator iter_cur, iter_end;
	iter_cur = Connection.begin();
	iter_end = Connection.end();
	while(iter_cur != iter_end){
		IP.push_back(iter_cur->second.IP);
		++iter_cur;
	}
}

const std::string download::name()
{
	//if download visible this should be defined in derived
	return "FUNCTION NOT SET IN DERIVED";
}

unsigned int download::percent_complete()
{
	return 0;
}

unsigned int download::speed()
{
	return Speed_Calculator.speed();
}

const boost::uint64_t download::size()
{
	return 0;
}

void download::register_connection(const download_connection & DC)
{
	Connection.insert(std::make_pair(DC.socket, server_info(DC.IP)));
}

void download::unregister_connection(const int & socket)
{
	Connection.erase(socket);
}

bool download::visible()
{
	return false;
}

void download::update_speed(const int & socket, const int & n_bytes)
{
	std::map<int, server_info>::iterator iter = Connection.find(socket);
	assert(iter != Connection.end());
	iter->second.Speed_Calculator.update(n_bytes);
	Speed_Calculator.update(n_bytes);
}
