#include "download_file_conn.h"

download_file_conn::download_file_conn(download * download_in, std::string server_IP_in, unsigned int file_ID_in)
{
	Download = download_in;
	server_IP = server_IP_in;
	file_ID = file_ID_in;

	Download_Speed.push_front(std::make_pair(0, 0));
	Download_Speed.push_front(std::make_pair(time(0), 0));
}

void download_file_conn::calculate_speed(const unsigned int & packet_size)
{
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

const unsigned int & download_file_conn::speed()
{
	return download_speed;
}
