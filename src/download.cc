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

void download::IP_list(std::vector<std::string> & list)
{
	std::map<int, download_conn *>::iterator iter_cur, iter_end;
	iter_cur = Connection.begin();
	iter_end = Connection.end();
	while(iter_cur != iter_end){
		list.push_back(iter_cur->second->server_IP);
		++iter_cur;
	}
}

unsigned int download::speed()
{
	return Speed_Calculator.speed();
}

void download::reg_conn(const int & socket, download_conn * Download_conn)
{
	Connection.insert(std::make_pair(socket, Download_conn));
}

void download::unreg_conn(const int & socket)
{
	Connection.erase(socket);
}
