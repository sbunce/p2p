//std
#include <fstream>
#include <iostream>
#include <sstream>
#include <time.h>

#include "conversion.h"
#include "server.h"

server::server()
{
	connections = 0;
	send_pending = 0;
	FD_ZERO(&master_FDS);
	Server_Index.start();
}

void server::disconnect(const int & socket_FD)
{
#ifdef DEBUG
	std::cout << "info: server::disconnect(): server disconnecting socket number " << socket_FD << "\n";
#endif

	/*
	It is possible for disconnect() to get called more than once when a socket
	disconnects which necessitates this check before removing the socket from the
	set and decrementing connections.
	*/
	if(FD_ISSET(socket_FD, &master_FDS)){
		FD_CLR(socket_FD, &master_FDS);
		FD_CLR(socket_FD, &read_FDS);
		FD_CLR(socket_FD, &write_FDS);

		--connections;
		close(socket_FD);
	}

	//reduce FD_max if possible
	for(int x=FD_max; x != 0; --x){
		if(FD_ISSET(x, &master_FDS)){
			FD_max = x;
			break;
		}
	}

	{//begin lock scope
	boost::mutex::scoped_lock lock(SB_mutex);
	std::map<int, std::string>::iterator SB_iter = Send_Buff.find(socket_FD);
	if(SB_iter != Send_Buff.end()){
		--send_pending;
		Send_Buff.erase(socket_FD);
	}
	}//end lock scope

	{//begin lock scope
	boost::mutex::scoped_lock lock(RB_mutex);
	Recv_Buff.erase(socket_FD);
	}//end lock scope
}

void server::calculate_speed_file(const int & socket_FD, const int & file_ID, const int & file_block, const unsigned long & file_size, const std::string & file_path)
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(US_mutex);

	time_t current_time = time(0);

	//get IP of the socket
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	getpeername(socket_FD, (struct sockaddr*)&addr, &len);

	bool found = false;
	std::list<speed_element_file>::iterator iter_cur, iter_end;
	iter_cur = Upload_Speed.begin();
	iter_end = Upload_Speed.end();
	while(iter_cur != iter_end){

		//if element found update it
		if(iter_cur->client_IP == inet_ntoa(addr.sin_addr) && iter_cur->file_ID == file_ID){

			if(iter_cur->Speed.back().first == current_time){
				iter_cur->Speed.back().second += global::P_BLS_SIZE;
			}
			else{
				iter_cur->Speed.push_back(std::make_pair(current_time, global::P_BLS_SIZE));
			}

			if(iter_cur->Speed.size() > global::SPEED_AVERAGE + 1){
				iter_cur->Speed.pop_front();
			}

			iter_cur->file_block = file_block;
			found = true;
			break;
		}
		++iter_cur;
	}

	//make a new element if there is no speed element for this client
	if(!found){
		Upload_Speed.push_back(speed_element_file(
			file_path.substr(file_path.find_last_of("/")+1),
			inet_ntoa(addr.sin_addr),
			file_ID,
			file_size,
			file_block
		));
		Upload_Speed.back().Speed.push_back(std::make_pair(current_time, global::P_BLS_SIZE));
	}
	}//end lock scope
}

int server::get_total_speed()
{
	int total_bytes = 0;

	{//begin lock scope
	boost::mutex::scoped_lock lock(US_mutex);
	std::list<speed_element_file>::iterator iter_cur, iter_end;
	iter_cur = Upload_Speed.begin();
	iter_end = Upload_Speed.end();
	while(iter_cur != iter_end){
		//add up bytes for SPEED_AVERAGE seconds (except for incomplete second)
		for(int x=0; x<iter_cur->Speed.size() - 1; ++x){
			total_bytes += iter_cur->Speed[x].second;
		}
		++iter_cur;
	}
	}//end lock scope

	int speed = total_bytes / global::SPEED_AVERAGE;

	return speed;
}

bool server::get_upload_info(std::vector<info_buffer> & upload_info)
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(US_mutex);

	time_t current_time = time(0);

	//remove completed downloads from Upload_Speed
	std::list<speed_element_file>::iterator iter_cur, iter_end;
	iter_cur = Upload_Speed.begin();
	iter_end = Upload_Speed.end();
	while(iter_cur != iter_end){
		if(iter_cur->Speed.back().first < (int)current_time - global::COMPLETE_REMOVE){
			iter_cur = Upload_Speed.erase(iter_cur);
		}
		else{
			++iter_cur;
		}
	}

	if(Upload_Speed.empty()){
		return false;
	}

	iter_cur = Upload_Speed.begin();
	iter_end = Upload_Speed.end();
	while(iter_cur != iter_end){
		float percent = ((iter_cur->file_block * (global::P_BLS_SIZE - 1)) / (float)iter_cur->file_size) * 100;
		if(percent > 100){
			percent = 100;
		}

		//add up bytes for SPEED_AVERAGE seconds (except for incomplete second)
		int total_bytes = 0;
		for(int x=0; x<iter_cur->Speed.size() - 1; ++x){
			total_bytes += iter_cur->Speed[x].second;
		}

		//take average
		total_bytes = total_bytes / global::SPEED_AVERAGE;

		std::ostringstream file_ID_temp;
		file_ID_temp << iter_cur->file_ID;

		upload_info.push_back(info_buffer(
			iter_cur->client_IP,
			file_ID_temp.str(),
			iter_cur->file_name,
			iter_cur->file_size,
			total_bytes,
			(int)percent
		));

		++iter_cur;
	}
	}//end lock scope

	return true;
}

bool server::is_indexing()
{
	return Server_Index.is_indexing();
}

bool server::new_conn(const int & listener)
{
	struct sockaddr_in remoteaddr;
	socklen_t len = sizeof(remoteaddr);

	//make a new socket for incoming connection
	int new_FD = accept(listener, (struct sockaddr *)&remoteaddr, &len);

	if(new_FD == -1){
		perror("accept");
		return false;
	}

	//make sure the client isn't already connected
	std::string new_IP(inet_ntoa(remoteaddr.sin_addr));
	sockaddr_in temp_addr;
	for(int socket_FD = 0; socket_FD <= FD_max; ++socket_FD){
		if(FD_ISSET(socket_FD, &master_FDS)){
			getpeername(socket_FD, (struct sockaddr*)&temp_addr, &len);
			if(strcmp(new_IP.c_str(), inet_ntoa(temp_addr.sin_addr)) == 0){
#ifdef DEBUG
				std::cout << "error: server::new_conn(): client " << new_IP << " attempted multiple connections\n";
#endif
				close(new_FD);
				return false;
			}
		}
	}

	if(connections <= global::MAX_CONNECTIONS){
		++connections;

		{//begin lock scope
		boost::mutex::scoped_lock lock(SB_mutex);
		Send_Buff.insert(std::make_pair(new_FD, std::string()));
		}//end lock scope

		{//begin lock scope
		boost::mutex::scoped_lock lock(RB_mutex);
		Recv_Buff.insert(std::make_pair(new_FD, std::string()));
		}//end lock scope

		FD_SET(new_FD, &master_FDS);

		//make sure FD_max is correct, resize buffers if needed
		if(new_FD > FD_max){
			FD_max = new_FD;
		}

		#ifdef DEBUG
		std::cout << "info: server::new_conn(): " << inet_ntoa(remoteaddr.sin_addr) << " socket " << new_FD << " connected\n";
		#endif
	}

	return true;
}

void server::main_thread()
{
	//set listening socket
	int listener;
	if((listener = socket(PF_INET, SOCK_STREAM, 0)) == -1){
		perror("socket");
		exit(1);
	}

	//reuse port if already in use
	//if this isn't done it can take up to a minute for port to be de-allocated
	int yes = 1;
	if(setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
		perror("setsockopt");
		exit(1);
	}

	struct sockaddr_in myaddr;                 //server address
	myaddr.sin_family = AF_INET;               //set ipv4
	myaddr.sin_addr.s_addr = INADDR_ANY;       //set to listen on all available interfaces
	myaddr.sin_port = htons(global::P2P_PORT); //set listening port
	memset(&(myaddr.sin_zero), '\0', 8);

	//set listener info to what we set above
	if(bind(listener, (struct sockaddr *)&myaddr, sizeof(myaddr)) == -1){
		perror("bind");
		exit(1);
	}

	//start listener socket listening
	if(listen(listener, 10) == -1){
		perror("listen");
		exit(1);
	}

	FD_SET(listener, &master_FDS);
	FD_max = listener;
	char recv_buff[global::S_MAX_SIZE*global::PIPELINE_SIZE];
   int n_bytes;

	#ifdef DEBUG
	std::cout << "info: server::start_thread(): server created listener socket number " << listener << "\n";
	#endif

	while(true){
		read_FDS = master_FDS;
		write_FDS = master_FDS;
		/*
		This if(send_pending) exists to save a LOT of CPU time by not checking
		if sockets are ready to write when we don't need to write anything.
		*/
		if(send_pending != 0){
			if((select(FD_max+1, &read_FDS, &write_FDS, NULL, NULL)) == -1){
				//gprof will send PROF signal, this will ignore it
				if(errno != EINTR){
					perror("select");
					exit(1);
				}
			}
		}
		else{
			if((select(FD_max+1, &read_FDS, NULL, NULL, NULL)) == -1){
				//gprof will send PROF signal, this will ignore it
				if(errno != EINTR){
					perror("select");
					exit(1);
				}
			}
		}

		for(int socket_FD = 0; socket_FD <= FD_max; ++socket_FD){
			if(FD_ISSET(socket_FD, &read_FDS)){
				if(socket_FD == listener){ //new client connected
					new_conn(socket_FD);
				}
				else{ //existing socket sending data
					if((n_bytes = recv(socket_FD, recv_buff, global::S_MAX_SIZE*global::PIPELINE_SIZE, 0)) <= 0){
						#ifdef DEBUG
						if(n_bytes == -1){
							perror("recv");
						}
						#endif
						disconnect(socket_FD);
					}
					else{ //incoming data from client socket
						process_request(socket_FD, recv_buff, n_bytes);
					}
				}
			}

			//do not check for writes on listener, there is no corresponding Send_Buff element
			if(FD_ISSET(socket_FD, &write_FDS) && socket_FD != listener){
				std::map<int, std::string>::iterator SB_iter = Send_Buff.find(socket_FD);
				if(!SB_iter->second.empty()){
					//MSG_NOSIGNAL needed because abrupt client disconnect causes SIGPIPE
					if((n_bytes = send(socket_FD, SB_iter->second.c_str(), SB_iter->second.size(), MSG_NOSIGNAL)) < 0){
						disconnect(socket_FD);
					}
					else{ //remove bytes sent from buffer
						SB_iter->second.erase(0, n_bytes);
						if(SB_iter->second.empty()){
							--send_pending;
						}
					}
				}
			}
		}
	}
}

int server::prepare_file_block(std::map<int, std::string>::iterator & SB_iter, const int & socket_FD, const int & file_ID, const int & block_number)
{
	int start_size = SB_iter->second.size();

	//get file_size/filePath that corresponds to file_ID
	unsigned long file_size;
	std::string file_path;
	if(!Server_Index.file_info(file_ID, file_size, file_path)){
		//file was not found
		SB_iter->second += global::P_FNF;

		std::cout << "fatal error: server::prepare_file_block() couldn't find file in index" << std::endl;
		exit(1);
		return 0;
	}

	//check for valid file block request
	if(block_number*(global::P_BLS_SIZE - 1) > file_size){
		//a block past the end of file was requested
		SB_iter->second += global::P_DNE;

		std::cout << "fatal error: server::prepare_file_block() invalid block number requested" << std::endl;
		exit(1);
		return 0;
	}

	SB_iter->second += global::P_BLS;
	std::ifstream fin(file_path.c_str());
	if(fin.is_open()){
		//seek to the file_block the client wants (-1 for command space)
		fin.seekg(block_number*(global::P_BLS_SIZE - 1));

		//fill the buffer
		char ch;
		while(fin.get(ch)){
			SB_iter->second += ch;
			if(SB_iter->second.size() - start_size == global::P_BLS_SIZE){
				break;
			}
		}
		fin.close();
	}
	else{
		std::cout << "fatal error: server::prepare_file_block() couldn't open file" << std::endl;
		exit(1);
	}

	//update speed calculation (assumes there is a response)
	calculate_speed_file(socket_FD, file_ID, block_number, file_size, file_path);

	return 0;
}

void server::process_request(const int & socket_FD, char recv_buff[], const int & n_bytes)
{
	std::map<int, std::string>::iterator RB_iter = Recv_Buff.find(socket_FD);
	RB_iter->second.append(recv_buff, n_bytes);

	//disconnect servers that over-pipeline
	if(RB_iter->second.size() > global::S_MAX_SIZE*global::PIPELINE_SIZE){
		std::cout << "error: server::process_request() detected abusive client" << std::endl;
		disconnect(socket_FD);
	}

	//process recv buffer until it's empty or it contains no complete requests
	bool send_pending_temp = false;
	while(RB_iter->second.size()){
		if(RB_iter->second[0] == global::P_SBL && n_bytes >= global::P_SBL_SIZE){
			std::map<int, std::string>::iterator SB_iter = Send_Buff.find(socket_FD);
			int file_ID = conversion::decode_int(RB_iter->second.substr(1,4));
			int block_number = conversion::decode_int(RB_iter->second.substr(5,4));
			prepare_file_block(SB_iter, socket_FD, file_ID, block_number);
			RB_iter->second.erase(0, global::P_SBL_SIZE);
			send_pending_temp = true;
		}
		else{
			break;
		}
	}

	if(send_pending_temp){
		++send_pending;
	}
}

void server::start()
{
	boost::thread T(boost::bind(&server::main_thread, this));
}

