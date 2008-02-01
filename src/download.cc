#include "download.h"

download::download()
{
	download_speed = 0;
/*
	Download_Speed.push_front(std::make_pair(0, 0));
	Download_Speed.push_front(std::make_pair(time(0), 0));
*/
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

void download::calculate_speed(const unsigned int & packet_size)
{
	/*
	This function keeps track of how many bytes are downloaded in one second. It
	has a vector that keeps track of bytes in the current second and bytes in the
	previous second. Because the current second will always be incomplete, the
	previous second will be considered the download speed.
	*/
	unsigned int current_time = time(0);

	if(Download_Speed.empty()){
		Download_Speed.push_front(std::make_pair(current_time, 0));
	}

	if(Download_Speed.front().first != current_time){
		//on a new second
		Download_Speed.push_front(std::make_pair(current_time, packet_size));
	}
	else{
		Download_Speed.front().second += packet_size;
	}

	//the current second is never counted in the average because it's incomplete
	if(Download_Speed.size() > global::SPEED_AVERAGE + 1){
		Download_Speed.pop_back();
	}

	unsigned int total_bytes = 0;
	std::deque<std::pair<unsigned int, unsigned int> >::reverse_iterator iter_cur, iter_end;
	iter_cur = Download_Speed.rbegin();
	iter_end = Download_Speed.rend();
	int count = 0;
	while(iter_cur != iter_end && iter_cur->first != current_time){
		total_bytes += iter_cur->second;
		++count;
		++iter_cur;
	}

	if(count != 0){
		download_speed = total_bytes / count;
	}
	else{
		download_speed = 0;
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

const unsigned int & download::speed()
{
	return download_speed;
}

void download::reg_conn(const int & socket, download_conn * Download_conn)
{
	Connection.insert(std::make_pair(socket, Download_conn));
}

void download::unreg_conn(const int & socket)
{
	Connection.erase(socket);
}
