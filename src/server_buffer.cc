#include "server_buffer.h"

std::map<int, server_buffer *> server_buffer::Server_Buffer;
boost::mutex server_buffer::Mutex;
volatile int server_buffer::send_pending(0);

server_buffer::server_buffer()
{
	/*
	The server_buffer is used in a std::map. If std::map::[] is ever used to
	locate a client_buffer that doesn't exist the default ctor will be called and
	the program will be killed.
	*/
	logger::debug(LOGGER_P1,"improperly constructed server_buffer");
	exit(1);
}

server_buffer::server_buffer(
	const int & socket_FD_in,
	const std::string & IP_in
):
	socket_FD(socket_FD_in),
	IP(IP_in),
	exchange_key(true)
{
	#ifdef CORRUPT_BLOCKS
	srand(time(NULL));
	#endif

	send_buff.reserve(global::C_MAX_SIZE * global::PIPELINE_SIZE);
	recv_buff.reserve(global::S_MAX_SIZE * global::PIPELINE_SIZE);

	process_send.reserve(global::C_MAX_SIZE);
	process_request.reserve(global::S_MAX_SIZE);

	for(int x=0; x<256; ++x){
		Slot[x] = NULL;
	}
}

server_buffer::~server_buffer()
{
	for(int x=0; x<256; ++x){
		if(Slot[x] != NULL){
			delete Slot[x];
		}
	}
}

void server_buffer::close_slot(const std::string & request)
{
	if(Slot[(int)(unsigned char)request[1]] != NULL){
		delete Slot[(int)(unsigned char)request[1]];
		Slot[(int)(unsigned char)request[1]] = NULL;
	}
}

bool server_buffer::create_slot(char & slot_ID, const std::string & hash, const boost::uint64_t & size, const std::string & path, const slot_type ST)
{
	for(int x=0; x<256; ++x){
		if(Slot[x] == NULL){
			slot_ID = (char)x;
			Slot[x] = new slot_element(hash, size, path, ST);
			return true;
		}
	}
	return false;
}

server_buffer::slot_element * server_buffer::get_slot_element(const char & slot_ID)
{
	if(Slot[(int)(unsigned char)slot_ID] != NULL){
		return Slot[(int)(unsigned char)slot_ID];
	}else{
		return NULL;
	}
}

void server_buffer::process(char * buff, const int & n_bytes)
{
	if(exchange_key){
		prime_remote_result.append(buff, n_bytes);
		if(prime_remote_result.size() == global::DH_KEY_SIZE*2){
			Encryption.set_prime(prime_remote_result.substr(0,global::DH_KEY_SIZE));
			Encryption.set_remote_result(prime_remote_result.substr(global::DH_KEY_SIZE,global::DH_KEY_SIZE));
			send_buff += Encryption.get_local_result();
			prime_remote_result.clear();
			exchange_key = false;
		}
		if(prime_remote_result.size() > global::DH_KEY_SIZE*2){
			logger::debug(LOGGER_P1,"abusive client ", IP, " failed key negotation, too many bytes");
			DB_blacklist::add(IP);
		}
		return;
	}

	//decrypt received bytes
	Encryption.crypt_recv(buff, n_bytes);
	recv_buff.append(buff, n_bytes);

	//disconnect clients that have pipelined more than is allowed
	if(recv_buff.size() > global::S_MAX_SIZE*global::PIPELINE_SIZE){
		logger::debug(LOGGER_P1,"server ",IP," over pipelined");
		DB_blacklist::add(IP);
		return;
	}

	//slice off one command at a time and process
	while(recv_buff.size()){
		process_send.clear();
		process_request.clear();

		if(recv_buff[0] == global::P_CLOSE_SLOT && recv_buff.size() >= global::P_CLOSE_SLOT_SIZE){
			process_request = recv_buff.substr(0, global::P_CLOSE_SLOT_SIZE);
			recv_buff.erase(0, global::P_CLOSE_SLOT_SIZE);
			close_slot(process_request);
		}else if(recv_buff[0] == global::P_REQUEST_SLOT_HASH && recv_buff.size() >= global::P_REQUEST_SLOT_HASH_SIZE){
			process_request = recv_buff.substr(0, global::P_REQUEST_SLOT_HASH_SIZE);
			recv_buff.erase(0, global::P_REQUEST_SLOT_HASH_SIZE);
			request_slot_hash(process_request, process_send);
		}else if(recv_buff[0] == global::P_REQUEST_SLOT_FILE && recv_buff.size() >= global::P_REQUEST_SLOT_FILE_SIZE){
			process_request = recv_buff.substr(0, global::P_REQUEST_SLOT_FILE_SIZE);
			recv_buff.erase(0, global::P_REQUEST_SLOT_FILE_SIZE);
			request_slot_file(process_request, process_send);
		}else if(recv_buff[0] == global::P_SEND_BLOCK && recv_buff.size() >= global::P_SEND_BLOCK_SIZE){
			process_request = recv_buff.substr(0, global::P_SEND_BLOCK_SIZE);
			recv_buff.erase(0, global::P_SEND_BLOCK_SIZE);
			send_block(process_request, process_send);
		}else{
			//no full command left to slice off
			break;
		}

		Encryption.crypt_send(process_send);
		send_buff += process_send;
	}
}

void server_buffer::request_slot_hash(const std::string & request, std::string & send)
{
	std::string hash_hex = convert::binary_to_hex(request.substr(1,20));
	std::string hash_tree_path = global::HASH_DIRECTORY+hash_hex;
	std::fstream fin(hash_tree_path.c_str(), std::ios::in | std::ios::ate);
	if(fin.is_open()){
		//hash tree exists, make slot
		boost::uint64_t size = fin.tellg();
		char slot_ID;
		if(create_slot(slot_ID, hash_hex, size, hash_tree_path, SLOT_HASH_TREE)){
			logger::debug(LOGGER_P1,"granting hash slot ",(int)(unsigned char)slot_ID, " to ",IP);
			send += global::P_SLOT_ID;
			send += slot_ID;
		}else{
			logger::debug(LOGGER_P1,"server ",IP," requested more than 256 slots");
			DB_blacklist::add(IP);
		}
	}else{
		//hash tree doesn't exist
		logger::debug(LOGGER_P1,IP," requested hash that doesn't exist");
		send += global::P_ERROR;
	}
}

void server_buffer::make_slot_file(const std::string & root_hash_hex, const boost::uint64_t & size, const std::string & path, std::string & send)
{
	char slot_ID;
	if(create_slot(slot_ID, root_hash_hex, size, path, SLOT_FILE)){
		logger::debug(LOGGER_P1,"granting file slot ",(int)(unsigned char)slot_ID, " to ",IP);
		send += global::P_SLOT_ID;
		send += slot_ID;
		update_slot_speed(slot_ID, global::P_REQUEST_SLOT_FILE_SIZE);
	}else{
		logger::debug(LOGGER_P1,"server ",IP," requested more than 256 slots");
		DB_blacklist::add(IP);
	}
}

void server_buffer::request_slot_file(const std::string & request, std::string & send)
{
	boost::uint64_t size;
	std::string path;
	std::string root_hash_hex = convert::binary_to_hex(request.substr(1,20));
	if(DB_Share.lookup_hash(root_hash_hex, path, size)){
		//hash found in share, create slot
		make_slot_file(root_hash_hex, size, path, send);
	}else{
		//file not in share, check to see if it's downloading
		if(client_server_bridge::is_downloading_file(root_hash_hex, path, size)){
			//file is downloading
			make_slot_file(root_hash_hex, size, path, send);
		}else{
			//hash not found in share, send error
			logger::debug(LOGGER_P1,IP," requested hash that doesn't exist");
			send += global::P_ERROR;
		}
	}
}

void server_buffer::send_block(const std::string & request, std::string & send)
{
	slot_element * SE;
	if(SE = get_slot_element(request[1])){
		boost::uint64_t block_number = convert::decode<boost::uint64_t>(request.substr(2, 8));
		client_server_bridge::download_mode DM = client_server_bridge::is_downloading(SE->hash);
		if(DM == client_server_bridge::DOWNLOAD_HASH_TREE && SE->Slot_Type == SLOT_HASH_TREE){
			//client requested a block from a hash tree the client is currently downloadeding
			send += global::P_WAIT;
			update_slot_speed(request[1], global::P_WAIT_SIZE);
std::cout << "HASH TREE UPLOAD WHILE DOWNLOAD NOT IMPLEMENTED\n";
			return;
		}else if(DM == client_server_bridge::DOWNLOAD_FILE && SE->Slot_Type == SLOT_FILE){
			//client requested a block from a file the client is currently downloading
			if(!client_server_bridge::is_available(SE->hash, block_number)){
std::cout << "sending P_WAIT\n";
				//block is not available to be sent, tell the client to wait
				send += global::P_WAIT;
				update_slot_speed(request[1], global::P_WAIT_SIZE);
				return;
			}
		}

		update_slot_percent_complete(request[1], block_number);
		send += global::P_BLOCK;
		std::ifstream fin(SE->path.c_str());
		if(fin.is_open()){
			//seek to the file_block the client wants (-1 for command space)
			fin.seekg(block_number * global::FILE_BLOCK_SIZE);
			fin.read(send_block_buff, global::FILE_BLOCK_SIZE);

			#ifdef CORRUPT_BLOCKS
			if(rand() % 20 == 0){
				send_block_buff[0] = (char)0;
			}
			#endif

			send.append(send_block_buff, fin.gcount());
			update_slot_speed(request[1], fin.gcount());
		}else{
			//hash not found in share, send error
			logger::debug(LOGGER_P1,"server could not open file: ",SE->path);
			send += global::P_ERROR;
		}
	}else{
		logger::debug(LOGGER_P1,IP," sent invalid slot ID ",(int)(unsigned char)request[1]);
		DB_blacklist::add(IP);
	}
}

void server_buffer::update_slot_percent_complete(char slot_ID, const boost::uint64_t & block_number)
{
	if(Slot[(int)(unsigned char)slot_ID] != NULL){
		if(block_number != 0){
			Slot[(int)(unsigned char)slot_ID]->percent_complete =
				(int)(((double)(block_number * global::FILE_BLOCK_SIZE) / (double)Slot[(int)(unsigned char)slot_ID]->size)*100);
			if(Slot[(int)(unsigned char)slot_ID]->percent_complete > 100){
				Slot[(int)(unsigned char)slot_ID]->percent_complete = 100;
			}
		}
	}
}

void server_buffer::update_slot_speed(char slot_ID, unsigned int bytes)
{
	if(Slot[(int)(unsigned char)slot_ID] != NULL){
		Slot[(int)(unsigned char)slot_ID]->Speed_Calculator.update(bytes);
	}
}

void server_buffer::uploads(std::vector<upload_info> & info)
{
	for(int x=0; x<256; ++x){
		if(Slot[x] != NULL){
			info.push_back(upload_info(
				Slot[x]->hash,
				IP,
				Slot[x]->size,
				Slot[x]->path,
				Slot[x]->Speed_Calculator.speed(),
				Slot[x]->percent_complete
			));
		}
	}
}
