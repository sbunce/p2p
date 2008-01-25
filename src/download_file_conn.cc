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
	while(iter_cur != iter_end && iter_cur->first != current_time){
		total_bytes += iter_cur->second;
		++iter_cur;
	}

	download_speed = total_bytes / Download_Speed.size();
}

const unsigned int & download_file_conn::speed()
{
	return download_speed;
}
