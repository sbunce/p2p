#include "server_buffer.hpp"

//BEGIN STATIC
boost::mutex server_buffer::Mutex;
std::map<int, boost::shared_ptr<server_buffer> > server_buffer::Server_Buffer;
int server_buffer::send_pending(0);

void server_buffer::current_uploads(std::vector<upload_info> & info)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::map<int, boost::shared_ptr<server_buffer> >::iterator iter_cur, iter_end;
	iter_cur = Server_Buffer.begin();
	iter_end = Server_Buffer.end();
	while(iter_cur != iter_end){
		iter_cur->second->uploads(info);
		++iter_cur;
	}
}

void server_buffer::erase(const int & socket_FD)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::map<int, boost::shared_ptr<server_buffer> >::iterator iter = Server_Buffer.find(socket_FD);
	if(iter != Server_Buffer.end()){
		if(!iter->second->send_buff.empty()){
			//send_buff contains data, decrement send_pending
			--send_pending;
		}
		Server_Buffer.erase(iter);
	}
}

bool server_buffer::get_send_buff(const int & socket_FD, const int & max_bytes, std::string & destination)
{
	boost::mutex::scoped_lock lock(Mutex);
	destination.clear();
	std::map<int, boost::shared_ptr<server_buffer> >::iterator iter = Server_Buffer.find(socket_FD);
	if(iter == Server_Buffer.end()){
		return false;
	}else{
		int size;
		if(max_bytes > iter->second->send_buff.size()){
			size = iter->second->send_buff.size();
		}else{
			size = max_bytes;
		}
		destination.assign(iter->second->send_buff.data(), size);
		return !destination.empty();
	}
}

int server_buffer::get_send_pending()
{
	boost::mutex::scoped_lock lock(Mutex);
	return send_pending;
}

void server_buffer::new_connection(const int & socket_FD, const std::string & IP)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::map<int, boost::shared_ptr<server_buffer> >::iterator iter = Server_Buffer.find(socket_FD);
	if(iter == Server_Buffer.end()){
		Server_Buffer.insert(std::make_pair(socket_FD, new server_buffer(socket_FD, IP)));
	}
}

bool server_buffer::post_send(const int & socket_FD, const int & n_bytes)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::map<int, boost::shared_ptr<server_buffer> >::iterator iter = Server_Buffer.find(socket_FD);
	if(iter != Server_Buffer.end()){
		iter->second->send_buff.erase(0, n_bytes);
		if(n_bytes && iter->second->send_buff.empty()){
			//buffer went from empty to non-empty
			--send_pending;
		}

		if(iter->second->disconnect_on_empty && iter->second->send_buff.empty()){
			return false;
		}else{
			return true;
		}
	}else{
		//disconnect if server_buffer no longer exists
		return false;
	}
}

void server_buffer::post_recv(const int & socket_FD, char * recv_buff, const int & n_bytes)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::map<int, boost::shared_ptr<server_buffer> >::iterator iter = Server_Buffer.find(socket_FD);
	if(iter != Server_Buffer.end()){
		bool initial_empty = iter->second->send_buff.size() == 0;
		iter->second->recv_buff_process(recv_buff, n_bytes);
		if(initial_empty && iter->second->send_buff.size() != 0){
			//buffer went from empty to non-empty, signal that a send needs to be done
			++send_pending;
		}
	}
}
//END STATIC

server_buffer::server_buffer()
{
	LOGGER << "improperly constructed server_buffer";
	exit(1);
}

server_buffer::server_buffer(
	const int & socket_FD_in,
	const std::string & IP_in
):
	socket_FD(socket_FD_in),
	IP(IP_in),
	disconnect_on_empty(false),
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
	}else{
		LOGGER << IP << " attempted to close slot it didn't have open";
		DB_Blacklist.add(IP);
	}
}

bool server_buffer::find_empty_slot(const std::string & root_hash, int & slot_num)
{
	//check to make sure slot for file not already requested
	for(int x=0; x<256; ++x){
		if(Slot[x] != NULL && Slot[x]->get_hash() == root_hash){
			LOGGER << IP << " requested slot for " << root_hash << " twice";
			DB_Blacklist.add(IP);
			return false;
		}
	}

	//find empty slot
	for(int x=0; x<256; ++x){
		if(Slot[x] == NULL){
			slot_num = x;
			return true;
		}
	}

	LOGGER << IP << " requested more than 256 slots";
	DB_Blacklist.add(IP);
	return false;
}

void server_buffer::recv_buff_process(char * buff, const int & n_bytes)
{
	if(IP.find("127.") != std::string::npos){
		//localhost obeys entirely different protocol
		if(disconnect_on_empty){
			//response already sent
			return;
		}

		recv_buff.append(buff, n_bytes);
		unsigned loc;
		if(recv_buff[0] == global::P_INFO && (loc = recv_buff.find("\n\r")) != std::string::npos){
			//http request ends with \n\r
			process_request = recv_buff.substr(0, loc);
			recv_buff.erase(0, loc);
			HTTP.process(process_request, send_buff);
			disconnect_on_empty = true;
		}

		if(recv_buff.size() > 1024*1024){
			//very generous sanity check for local recv buff
			recv_buff.clear();
		}
		return;
	}

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
			LOGGER << "abusive client " << IP << " failed key negotation, too many bytes";
			DB_Blacklist.add(IP);
		}
		return;
	}

	//decrypt received bytes
	Encryption.crypt_recv(buff, n_bytes);
	recv_buff.append(buff, n_bytes);

	//disconnect clients that have pipelined more than is allowed
	if(recv_buff.size() > global::S_MAX_SIZE*global::PIPELINE_SIZE){
		LOGGER << "server " << IP << " over pipelined";
		DB_Blacklist.add(IP);
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
		}else if(recv_buff[0] == global::P_BLOCK && recv_buff.size() >= global::P_BLOCK_TO_SERVER_SIZE){
			process_request = recv_buff.substr(0, global::P_BLOCK_TO_SERVER_SIZE);
			recv_buff.erase(0, global::P_BLOCK_TO_SERVER_SIZE);
			send_block(process_request, process_send);
		}else{
			//no full command left to slice off
			break;
		}

		//encrypt bytes to send
		Encryption.crypt_send(process_send);
		send_buff += process_send;
	}
}

void server_buffer::request_slot_hash(const std::string & request, std::string & send)
{
	std::string root_hash = convert::bin_to_hex(request.substr(1,20));

	//check if hash tree currently downloading
	boost::uint64_t file_size;
	if(DB_Download.lookup_hash(root_hash, file_size)){
		int slot_num;
		if(find_empty_slot(root_hash, slot_num)){
			LOGGER << "granting slot " << slot_num << " to " << IP;
			Slot[slot_num] = new slot_hash_tree(&IP, root_hash, file_size);
			send += global::P_SLOT_ID;
			send += (char)slot_num;
		}
		return;
	}

	//check if hash tree exists in share
	if(DB_Share.lookup_hash(root_hash, file_size)){
		int slot_num;
		if(find_empty_slot(root_hash, slot_num)){
			LOGGER << "granting slot " << slot_num << " to " << IP;
			Slot[slot_num] = new slot_hash_tree(&IP, root_hash, file_size);
			send += global::P_SLOT_ID;
			send += (char)slot_num;
		}
		return;
	}

	LOGGER << IP << " requested unavailable hash tree " << root_hash;
	send += global::P_ERROR;
}

void server_buffer::request_slot_file(const std::string & request, std::string & send)
{
	std::string root_hash = convert::bin_to_hex(request.substr(1,20));

	//check if file is currently downloading
	boost::uint64_t file_size;
	std::string file_path;
	if(DB_Download.lookup_hash(root_hash, file_path, file_size)){
		int slot_num;
		if(find_empty_slot(root_hash, slot_num)){
			//slot available
			LOGGER << "granting file slot " << slot_num << " to " << IP;
			Slot[slot_num] = new slot_file(&IP, root_hash, file_path, file_size);
			send += global::P_SLOT_ID;
			send += (char)slot_num;
		}
		return;
	}

	if(DB_Share.lookup_hash(root_hash, file_path, file_size)){
		int slot_num;
		if(find_empty_slot(root_hash, slot_num)){
			//slot available
			LOGGER << "granting file slot " << slot_num << " to " << IP;
			Slot[slot_num] = new slot_file(&IP, root_hash, file_path, file_size);
			send += global::P_SLOT_ID;
			send += (char)slot_num;
		}
		return;
	}

	LOGGER << IP << " requested unavailable hash tree " << root_hash;
	send += global::P_ERROR;
}

void server_buffer::send_block(const std::string & request, std::string & send)
{
	int slot_num = (int)(unsigned char)request[1];
	if(Slot[slot_num] != NULL){
		//valid slot
		Slot[slot_num]->send_block(request, send);
	}else{
		LOGGER << IP << " sent invalid slot ID " << slot_num;
		DB_Blacklist.add(IP);
	}
}

void server_buffer::uploads(std::vector<upload_info> & UI)
{
	for(int x=0; x<256; ++x){
		if(Slot[x] != NULL){
			Slot[x]->info(UI);
		}
	}
}
