#include "download.h"

download::download()
{
	download_speed = 0;
	Download_Speed.push_front(std::make_pair(0, 0));
	Download_Speed.push_front(std::make_pair(time(0), 0));
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

	if(Download_Speed.front().first != time(0)){
		//on a new second
		Download_Speed.pop_back();
		Download_Speed.push_front(std::make_pair(time(0), packet_size));
	}
	else{
		Download_Speed.front().second += packet_size;
	}

	download_speed = Download_Speed.back().second;
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
