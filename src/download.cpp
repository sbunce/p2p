#include "download.hpp"

download::download()
:
	bytes_received(0),
	cancel(false),
	visible(false)
{

}

download::~download()
{

}

int download::connection_count()
{
	return Connection.size();
}

bool download::is_cancelled()
{
	return cancel;
}

bool download::is_visible()
{
	return visible;
}

unsigned download::speed()
{
	if(Connection.empty()){
		return 0;
	}else{
		return Speed_Calculator.speed();
	}
}

void download::register_connection(const download_connection & DC)
{
	Connection.insert(std::make_pair(DC.socket, server_info(DC.IP)));
}

void download::servers(std::vector<std::string> & IP, std::vector<unsigned> & speed)
{
	std::map<int, server_info>::iterator iter_cur, iter_end;
	iter_cur = Connection.begin();
	iter_end = Connection.end();
	while(iter_cur != iter_end){
		IP.push_back(iter_cur->second.IP);
		speed.push_back(iter_cur->second.Speed_Calculator.speed());
		++iter_cur;
	}
}

void download::unregister_connection(const int & socket)
{
	Connection.erase(socket);
}

void download::update_speed(const int & socket, const int & n_bytes)
{
	std::map<int, server_info>::iterator iter = Connection.find(socket);
	assert(iter != Connection.end());
	iter->second.Speed_Calculator.update(n_bytes);
	Speed_Calculator.update(n_bytes);
	bytes_received += n_bytes;
}
