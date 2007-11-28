#include "download.h"

download::download()
{

}

std::list<std::string> & download::get_IPs()
{
	get_IPs_list.clear();

	std::list<connection>::iterator iter_cur, iter_end;
	iter_cur = current_conns.begin();
	iter_end = current_conns.end();
	while(iter_cur != iter_end){
		get_IPs_list.push_back(iter_cur->server_IP);
		++iter_cur;
	}

	return get_IPs_list;
}

void download::reg_conn(std::string & server_IP)
{
	connection temp(server_IP);
	current_conns.push_back(temp);
}

void download::unreg_conn(const std::string & server_IP)
{
	std::list<connection>::iterator iter_cur, iter_end;
	iter_cur = current_conns.begin();
	iter_end = current_conns.end();
	while(iter_cur != iter_end){
		if(server_IP == iter_cur->server_IP){
			current_conns.erase(iter_cur);
			break;
		}
		++iter_cur;
	}
}
