#include "server.h"

server::server()
:
connections(0),
send_pending(0),
stop_threads(false),
total_speed(0),
threads(0)
{
	FD_ZERO(&master_FDS);
	Server_Index.start();
	Speed_Calculator.set_speed_limit(DB_Server_Preferences.get_speed_limit_uint());
}

void server::disconnect(const int & socket_FD)
{
	logger::debug(LOGGER_P1,"disconnecting socket ",socket_FD);

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
	for(int x = FD_max; x != 0; --x){
		if(FD_ISSET(x, &master_FDS)){
			FD_max = x;
			break;
		}
	}

	{//begin lock scope
	boost::mutex::scoped_lock lock(SB_mutex);
	std::map<int, send_buff_element>::iterator SB_iter = Send_Buff.find(socket_FD);
	if(!SB_iter->second.buff.empty()){
		--send_pending;
	}
	Send_Buff.erase(SB_iter);
	}//end lock scope

	{//begin lock scope
	boost::mutex::scoped_lock lock(RB_mutex);
	Recv_Buff.erase(socket_FD);
	}//end lock scope
}

void server::calculate_speed_file(std::string & client_IP, const unsigned int & file_ID, const unsigned int & file_block, const unsigned long & file_size, const std::string & file_path)
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(US_mutex);

	time_t current_time = time(0);

	bool found = false;
	std::list<speed_element_file>::iterator iter_cur, iter_end;
	iter_cur = Upload_Speed.begin();
	iter_end = Upload_Speed.end();
	while(iter_cur != iter_end){

		//if element found update it
		if(iter_cur->client_IP == client_IP && iter_cur->file_ID == file_ID){
			if(iter_cur->Speed.back().first == current_time){
				iter_cur->Speed.back().second += global::P_BLOCK_SIZE;
			}else{
				iter_cur->Speed.push_back(std::make_pair(current_time, global::P_BLOCK_SIZE));
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
			client_IP,
			file_ID,
			file_size,
			file_block
		));
		Upload_Speed.back().Speed.push_back(std::make_pair(current_time, global::P_BLOCK_SIZE));
	}
	}//end lock scope
}

int server::get_max_connections()
{
	return DB_Server_Preferences.get_max_connections();
}

void server::set_max_connections(int max_connections)
{
	DB_Server_Preferences.set_max_connections(max_connections);
}

std::string server::get_share_directory()
{
	return DB_Server_Preferences.get_share_directory();
}

void server::set_share_directory(const std::string & share_directory)
{
	DB_Server_Preferences.set_share_directory(share_directory);
}

std::string server::get_speed_limit()
{
	std::ostringstream sl;
	sl << Speed_Calculator.get_speed_limit() / 1024;
	return sl.str();
}

void server::set_speed_limit(const std::string & speed_limit)
{
	std::stringstream ss(speed_limit);
	unsigned int speed;
	ss >> speed;
	speed *= 1024;
	DB_Server_Preferences.set_speed_limit(speed);
	Speed_Calculator.set_speed_limit(speed);
}

int server::get_total_speed()
{
	Speed_Calculator.update(0);
	return Speed_Calculator.speed();
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
		}else{
			++iter_cur;
		}
	}

	if(Upload_Speed.empty()){
		return false;
	}

	iter_cur = Upload_Speed.begin();
	iter_end = Upload_Speed.end();
	while(iter_cur != iter_end){
		float percent = ((iter_cur->file_block * (global::P_BLOCK_SIZE - 1)) / (float)iter_cur->file_size) * 100;
		if(percent >= 99){
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
	sockaddr_in remoteaddr;
	socklen_t len = sizeof(remoteaddr);

	//make a new socket for incoming connection
	int new_FD = accept(listener, (sockaddr *)&remoteaddr, &len);

	if(new_FD == -1){
		perror("accept");
		return false;
	}

	//do not accept connections from localhost
	std::string new_IP(inet_ntoa(remoteaddr.sin_addr));
	if(new_IP.substr(0,3) == "127"){
		logger::debug(LOGGER_P1,"refusing connection from localhost");
		return false;
	}

	//make sure the client isn't already connected
	sockaddr_in temp_addr;
	for(int socket_FD = 0; socket_FD <= FD_max; ++socket_FD){
		if(FD_ISSET(socket_FD, &master_FDS)){
			getpeername(socket_FD, (sockaddr*)&temp_addr, &len);
			if(strcmp(new_IP.c_str(), inet_ntoa(temp_addr.sin_addr)) == 0){
				logger::debug(LOGGER_P1,"server ",new_IP," attempted multiple connections");
				close(new_FD);
				return false;
			}
		}
	}

	if(connections <= global::MAX_CONNECTIONS){
		++connections;

		{//begin lock scope
		boost::mutex::scoped_lock lock(SB_mutex);
		Send_Buff.insert(std::make_pair(new_FD, send_buff_element(new_IP)));
		}//end lock scope

		{//begin lock scope
		boost::mutex::scoped_lock lock(RB_mutex);
		Recv_Buff.insert(std::make_pair(new_FD, std::string()));
		Recv_Buff[new_FD].reserve(global::S_MAX_SIZE * global::PIPELINE_SIZE);
		}//end lock scope

		FD_SET(new_FD, &master_FDS);

		//make sure FD_max is correct, resize buffers if needed
		if(new_FD > FD_max){
			FD_max = new_FD;
		}

		logger::debug(LOGGER_P1,"client ",inet_ntoa(remoteaddr.sin_addr)," socket ",new_FD," connected");
	}

	return true;
}

void server::main_thread()
{
	++threads;

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

	logger::debug(LOGGER_P1,"created listening socket ",listener);

	int send_limit = 0; //how many bytes can be sent on an iteration
	std::map<int, send_buff_element>::iterator SB_iter;
	while(true){
		if(stop_threads){
			break;
		}

		/*
		This if(send_pending) exists to saves CPU time by not checking if sockets
		are ready to write when we don't need to write anything.
		*/
		if(send_pending != 0){
			read_FDS = master_FDS;
			write_FDS = master_FDS;
			if((select(FD_max+1, &read_FDS, &write_FDS, NULL, NULL)) == -1){
				//gprof will send PROF signal, this will ignore it
				if(errno != EINTR){
					perror("server select");
					exit(1);
				}
			}
		}else{
			FD_ZERO(&write_FDS);
			read_FDS = master_FDS;
			if((select(FD_max+1, &read_FDS, NULL, NULL, NULL)) == -1){
				//gprof will send PROF signal, this will ignore it
				if(errno != EINTR){
					perror("server select");
					exit(1);
				}
			}
		}

		for(int socket_FD = 0; socket_FD <= FD_max; ++socket_FD){
			if(FD_ISSET(socket_FD, &read_FDS)){
				if(socket_FD == listener){ //new client connected
					new_conn(socket_FD);
				}else{ //existing socket sending data
					if((n_bytes = recv(socket_FD, recv_buff, global::S_MAX_SIZE*global::PIPELINE_SIZE, 0)) <= 0){
						if(n_bytes == -1){
							perror("server recv");
						}
						disconnect(socket_FD);
					}else{ //incoming data from client socket
						process_request(socket_FD, recv_buff, n_bytes);
					}
				}
			}

			//do not check for writes on listener, there is no corresponding Send_Buff element
			if(FD_ISSET(socket_FD, &write_FDS) && socket_FD != listener){
				SB_iter = Send_Buff.find(socket_FD);
				if(!SB_iter->second.buff.empty()){
					send_limit = Speed_Calculator.rate_control(SB_iter->second.buff.size());

					//MSG_NOSIGNAL needed because abrupt client disconnect causes SIGPIPE
					if((n_bytes = send(socket_FD, SB_iter->second.buff.c_str(), send_limit, MSG_NOSIGNAL)) < 0){
						if(n_bytes == -1){
							perror("server send");
						}
						disconnect(socket_FD);
					}else{ //remove bytes sent from buffer
						Speed_Calculator.update(n_bytes);
						SB_iter->second.buff.erase(0, n_bytes);
						if(SB_iter->second.buff.empty()){
							--send_pending;
						}
					}
				}
			}
		}
	}

	--threads;
}

void server::prepare_file_block(std::map<int, send_buff_element>::iterator & SB_iter, const int & socket_FD, const unsigned int & file_ID, const unsigned int & block_number)
{
	int start_size = SB_iter->second.buff.size();

	//get file_size/filePath that corresponds to file_ID
	unsigned long file_size;
	std::string file_path;
	if(!DB_Share.file_info(file_ID, file_size, file_path)){
		//file was not found
		SB_iter->second.buff += global::P_FILE_NOT_FOUND;
		return;
	}

	//check for valid file block request
	if(block_number*(global::P_BLOCK_SIZE - 1) > file_size){
		//a block past the end of file was requested
		SB_iter->second.buff += global::P_FILE_DOES_NOT_EXIST;
		return;
	}

	SB_iter->second.buff += global::P_BLOCK;
	std::ifstream fin(file_path.c_str());
	if(fin.is_open()){
		//seek to the file_block the client wants (-1 for command space)
		fin.seekg(block_number*(global::P_BLOCK_SIZE - 1));
		fin.read(prepare_file_block_buff, global::P_BLOCK_SIZE - 1);
		SB_iter->second.buff.append(prepare_file_block_buff, fin.gcount());
		fin.close();
	}else{
		logger::debug(LOGGER_P1,"could not open file \"",file_path,"\"");
		assert(false);
	}

	//update speed calculation (assumes there is a response)
	calculate_speed_file(SB_iter->second.client_IP, file_ID, block_number, file_size, file_path);
	return;
}

void server::process_request(const int & socket_FD, char recv_buff[], const int & n_bytes)
{
	std::map<int, std::string>::iterator RB_iter = Recv_Buff.find(socket_FD);
	RB_iter->second.append(recv_buff, n_bytes);

	//disconnect clients that have pipelined more than is allowed
	if(RB_iter->second.size() > global::S_MAX_SIZE*global::PIPELINE_SIZE){
		logger::debug(LOGGER_P1,"disconnecting abusive socket ",socket_FD);
		disconnect(socket_FD);

//blacklist needs to be done here
	}

	//needed to determine if the buffer went from empty to non-empty
	std::map<int, send_buff_element>::iterator SB_iter = Send_Buff.find(socket_FD);
	bool initial_empty_buff;
	if(SB_iter->second.buff.size() == 0){
		initial_empty_buff = true;
	}else{
		initial_empty_buff = false;
	}

	//process recv buffer until it's empty or it contains no complete requests
	while(RB_iter->second.size()){
		if(RB_iter->second[0] == global::P_SEND_BLOCK && n_bytes >= global::P_SEND_BLOCK_SIZE){
			unsigned int file_ID = Convert_uint32.decode(RB_iter->second.substr(1,4));
			unsigned int block_number = Convert_uint32.decode(RB_iter->second.substr(5,4));
			prepare_file_block(SB_iter, socket_FD, file_ID, block_number);
			RB_iter->second.erase(0, global::P_SEND_BLOCK_SIZE);
		}else{
			break;
		}
	}

	//if buff went from empty to non-empty a new send needs to be done
	if(initial_empty_buff && SB_iter->second.buff.size() != 0){
		++send_pending;
	}
}

void server::start()
{
	boost::thread T(boost::bind(&server::main_thread, this));
}

void server::stop()
{
	stop_threads = true;

	//get select() to return if it's blocking
	raise(SIGINT);
	while(threads){
		usleep(global::SPINLOCK_TIME);
	}
	Server_Index.stop();
}
