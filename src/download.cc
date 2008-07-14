#include "download.h"

download::download()
{

}

download::~download()
{

}

const std::string download::hash()
{
	return "FUNCTION NOT SET IN DERIVED";
}

void download::IP_list(std::vector<std::string> & IP)
{
	std::map<int, std::string>::iterator iter_cur, iter_end;
	iter_cur = Connection.begin();
	iter_end = Connection.end();
	while(iter_cur != iter_end){
		IP.push_back(iter_cur->second);
		++iter_cur;
	}
}

const std::string download::name()
{
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

const uint64_t download::size()
{
	return 0;
}

void download::register_connection(const download_connection & DC)
{
	Connection.insert(std::make_pair(DC.socket, DC.IP));
}

void download::unregister_connection(const int & socket)
{
	Connection.erase(socket);
}

bool download::visible()
{
	return false;
}
