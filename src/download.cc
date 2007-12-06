#include "download.h"

download::download()
{
	download_speed = 0;
}

void download::calculate_speed()
{
	time_t currentTime = time(0);

	bool updated = false;
	//if the vectors are empty
	if(Download_Second.empty()){
		Download_Second.push_back(currentTime);
		Second_Bytes.push_back(global::BUFFER_SIZE);
		updated = true;
	}

	//get rid of information that's older than what to average
	if(currentTime - Download_Second.front() >= global::SPEED_AVERAGE + 2){
		Download_Second.erase(Download_Second.begin());
		Second_Bytes.erase(Second_Bytes.begin());
	}

	//if still on the same second
	if(!updated && Download_Second.back() == currentTime){
		Second_Bytes.back() += global::BUFFER_SIZE;
		updated = true;
	}

	//no entry for current second, add one
	if(!updated){
		Download_Second.push_back(currentTime);
		Second_Bytes.push_back(global::BUFFER_SIZE);
	}

	//count up all the bytes excluding the first and last second
	int totalBytes = 0;
	for(int x = 1; x <= Download_Second.size() - 1; ++x){
		totalBytes += Second_Bytes[x];
	}

	//divide by the time
	download_speed = totalBytes / global::SPEED_AVERAGE;
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
