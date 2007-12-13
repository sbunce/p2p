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

const int & download::speed()
{
	return download_speed;
}

