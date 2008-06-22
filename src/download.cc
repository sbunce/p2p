#include "download.h"

download::download()
{

}

download::~download()
{
	std::map<int, download_conn *>::iterator iter_cur, iter_end;
	iter_cur = Connection.begin();
	iter_end = Connection.end();
	while(iter_cur != iter_end){
		delete iter_cur->second;
		++iter_cur;	
	}
}

void download::IP_list(std::vector<std::string> & IP)
{
	std::map<int, download_conn *>::iterator iter_cur, iter_end;
	iter_cur = Connection.begin();
	iter_end = Connection.end();
	while(iter_cur != iter_end){
		IP.push_back(iter_cur->second->IP);
		++iter_cur;
	}
}

unsigned int download::speed()
{
	return Speed_Calculator.speed();
}

void download::register_connection(download_conn * DC)
{
	Connection.insert(std::make_pair(DC->socket, DC));
}

void download::unregister_connection(const int & socket)
{
	Connection.erase(socket);
}
