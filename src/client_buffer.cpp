#include "client_buffer.hpp"

//BEGIN STATIC
boost::mutex client_buffer::Mutex;
std::map<int, boost::shared_ptr<client_buffer> > client_buffer::Client_Buffer;
std::set<locking_shared_ptr<download> > client_buffer::Unique_Download;
int client_buffer::send_pending(0);

bool client_buffer::add_connection(download_connection & DC)
{
//std::cout << "start 1\n";
	boost::mutex::scoped_lock lock(Mutex);
	std::map<int, boost::shared_ptr<client_buffer> >::iterator iter_cur, iter_end;
	iter_cur = Client_Buffer.begin();
	iter_end = Client_Buffer.end();
	while(iter_cur != iter_end){
		if(DC.IP == iter_cur->second->IP){
			DC.socket = iter_cur->second->socket;             //set socket of the discovered client_buffer
			DC.Download->register_connection(DC);             //register connection with download
			iter_cur->second->register_download(DC.Download); //register download with client_buffer
//std::cout << "end 1\n";
			return true;
		}
		++iter_cur;
	}
//std::cout << "end 1\n";
	return false;
}

void client_buffer::add_download(locking_shared_ptr<download> Download)
{
//std::cout << "start 2\n";
	boost::mutex::scoped_lock lock(Mutex);
	Unique_Download.insert(Download);
//std::cout << "end 2\n";
}

void client_buffer::current_downloads(std::vector<download_status> & info, std::string hash)
{
//std::cout << "start 3\n";
	boost::mutex::scoped_lock lock(Mutex);
	info.clear();

	if(hash.empty()){
		//info for all downloads
		std::set<locking_shared_ptr<download> >::iterator iter_cur, iter_end;
		iter_cur = Unique_Download.begin();
		iter_end = Unique_Download.end();
		while(iter_cur != iter_end){
			if((*iter_cur)->is_visible()){
				download_status Download_Status(
					(*iter_cur)->hash(),
					(*iter_cur)->name(),
					(*iter_cur)->size(),
					(*iter_cur)->speed(),
					(*iter_cur)->percent_complete()
				);
				(*iter_cur)->servers(Download_Status.IP, Download_Status.speed);
				info.push_back(Download_Status);
			}
			++iter_cur;
		}
	}else{
		//info for one download
		std::set<locking_shared_ptr<download> >::iterator iter_cur, iter_end;
		iter_cur = Unique_Download.begin();
		iter_end = Unique_Download.end();
		while(iter_cur != iter_end){
			if((*iter_cur)->hash() == hash){
				download_status Download_Status(
					(*iter_cur)->hash(),
					(*iter_cur)->name(),
					(*iter_cur)->size(),
					(*iter_cur)->speed(),
					(*iter_cur)->percent_complete()
				);
				(*iter_cur)->servers(Download_Status.IP, Download_Status.speed);
				info.push_back(Download_Status);
				break;
			}
			++iter_cur;
		}
	}
//std::cout << "end 3\n";
}

void client_buffer::erase(const int & socket_FD)
{
//std::cout << "start 4\n";
	boost::mutex::scoped_lock lock(Mutex);
	std::map<int, boost::shared_ptr<client_buffer> >::iterator iter = Client_Buffer.find(socket_FD);
	assert(iter != Client_Buffer.end());
	if(!iter->second->send_buff.empty()){
		//send_buff contains data, decrement send_pending
		--send_pending;
	}
	Client_Buffer.erase(iter);
//std::cout << "end 4\n";
}

bool client_buffer::is_downloading(locking_shared_ptr<download> Download)
{
//std::cout << "start 5\n";
	boost::mutex::scoped_lock lock(Mutex);
	std::set<locking_shared_ptr<download> >::iterator iter = Unique_Download.find(Download);
	return iter != Unique_Download.end();
//std::cout << "end 5\n";
}

bool client_buffer::is_downloading(const std::string & hash)
{
//std::cout << "start 6\n";
	boost::mutex::scoped_lock lock(Mutex);
	std::set<locking_shared_ptr<download> >::iterator iter_cur, iter_end;
	iter_cur = Unique_Download.begin();
	iter_end = Unique_Download.end();
	while(iter_cur != iter_end){
		if((*iter_cur)->hash() == hash){
//std::cout << "end 6\n";
			return true;
		}
		++iter_cur;
	}
//std::cout << "end 6\n";
	return false;
}

void client_buffer::find_complete(std::vector<locking_shared_ptr<download> > & complete)
{
//std::cout << "start 7\n";
	boost::mutex::scoped_lock lock(Mutex);
	std::set<locking_shared_ptr<download> >::iterator iter_cur, iter_end;
	iter_cur = Unique_Download.begin();
	iter_end = Unique_Download.end();
	while(iter_cur != iter_end){
		if((*iter_cur)->complete()){
			complete.push_back(*iter_cur);
		}
		++iter_cur;
	}
//std::cout << "end 7\n";
}

void client_buffer::find_empty(std::vector<int> & disconnect_sockets)
{
//std::cout << "start 8\n";
	boost::mutex::scoped_lock lock(Mutex);
	std::map<int, boost::shared_ptr<client_buffer> >::iterator iter_cur, iter_end;
	iter_cur = Client_Buffer.begin();
	iter_end = Client_Buffer.end();
	while(iter_cur != iter_end){
		if(iter_cur->second->empty()){
			disconnect_sockets.push_back(iter_cur->first);
		}
		++iter_cur;
	}
//std::cout << "end 8\n";
}

void client_buffer::find_timed_out(std::vector<int> & timed_out)
{
//std::cout << "start 9\n";
	boost::mutex::scoped_lock lock(Mutex);
	std::map<int, boost::shared_ptr<client_buffer> >::iterator iter_cur, iter_end;
	iter_cur = Client_Buffer.begin();
	iter_end = Client_Buffer.end();
	while(iter_cur != iter_end){
		if(time(0) - iter_cur->second->last_seen >= global::TIMEOUT){
			timed_out.push_back(iter_cur->first);
		}
		++iter_cur;
	}
//std::cout << "end 9\n";
}

void client_buffer::generate_requests()
{
//std::cout << "start 10\n";
	boost::mutex::scoped_lock lock(Mutex);
	std::map<int, boost::shared_ptr<client_buffer> >::iterator iter_cur, iter_end;
	iter_cur = Client_Buffer.begin();
	iter_end = Client_Buffer.end();
	while(iter_cur != iter_end){
		iter_cur->second->prepare_request();
		++iter_cur;
	}
//std::cout << "end 10\n";
}

int client_buffer::get_send_pending()
{
//std::cout << "start 11\n";
	boost::mutex::scoped_lock lock(Mutex);
	assert(send_pending >= 0);
	return send_pending;
//std::cout << "end 11\n";
}

std::string & client_buffer::get_send_buff(const int & socket_FD)
{
//std::cout << "start 12\n";
	boost::mutex::scoped_lock lock(Mutex);
	std::map<int, boost::shared_ptr<client_buffer> >::iterator iter = Client_Buffer.find(socket_FD);
	assert(iter != Client_Buffer.end());
	return iter->second->send_buff;
//std::cout << "end 12\n";
}

void client_buffer::new_connection(const download_connection & DC)
{
//std::cout << "start 13\n";
	boost::mutex::scoped_lock lock_1(Mutex);
	Client_Buffer.insert(std::make_pair(DC.socket, new client_buffer(DC.socket, DC.IP)));

	/*
	Add download to new client_buffer. It's possible that the download was
	ended before this function was called. When this happens there will be an
	empty client_buffer that will get cleaned up by the client.
	*/
	std::set<locking_shared_ptr<download> >::iterator iter = Unique_Download.find(DC.Download);
	if(iter != Unique_Download.end()){
		Client_Buffer[DC.socket]->register_download(DC.Download);
		DC.Download->register_connection(DC);
	}
//std::cout << "end 13\n";
}

void client_buffer::post_send(const int & socket_FD, const int & n_bytes)
{
//std::cout << "start 14\n";
	boost::mutex::scoped_lock lock(Mutex);
	std::map<int, boost::shared_ptr<client_buffer> >::iterator iter = Client_Buffer.find(socket_FD);
	assert(iter != Client_Buffer.end());
	iter->second->send_buff.erase(0, n_bytes);
	if(n_bytes && iter->second->send_buff.empty()){
		//buffer went from non-empty to empty
		--send_pending;
	}
//std::cout << "end 14\n";
}

void client_buffer::recv_buff_append(const int & socket_FD, char * buff, const int & n_bytes)
{
//std::cout << "start 15\n";
	boost::mutex::scoped_lock lock(Mutex);
	std::map<int, boost::shared_ptr<client_buffer> >::iterator iter = Client_Buffer.find(socket_FD);
	assert(iter != Client_Buffer.end());
	iter->second->recv_buff_append(buff, n_bytes);
//std::cout << "end 15\n";
}

void client_buffer::remove_download(locking_shared_ptr<download> Download)
{
//std::cout << "start 16\n";
	boost::mutex::scoped_lock lock(Mutex);
	Unique_Download.erase(Download);

	std::map<int, boost::shared_ptr<client_buffer> >::iterator iter_cur, iter_end;
	iter_cur = Client_Buffer.begin();
	iter_end = Client_Buffer.end();
	while(iter_cur != iter_end){
		iter_cur->second->terminate_download(Download);
		++iter_cur;
	}
//std::cout << "end 16\n";
}

void client_buffer::stop_download(const std::string & hash)
{
//std::cout << "start 17\n";
	boost::mutex::scoped_lock lock(Mutex);
	std::set<locking_shared_ptr<download> >::iterator iter_cur, iter_end;
	iter_cur = Unique_Download.begin();
	iter_end = Unique_Download.end();
	while(iter_cur != iter_end){
		if(hash == (*iter_cur)->hash()){
			(*iter_cur)->stop();
			break;
		}
		++iter_cur;
	}
//std::cout << "end 17\n";
}
//END STATIC

client_buffer::client_buffer()
{
	LOGGER << "improperly constructed client_buffer";
	exit(1);
}

client_buffer::client_buffer(
	const int & socket_in,
	const std::string & IP_in
):
	socket(socket_in),
	IP(IP_in),
	bytes_seen(0),
	last_seen(time(NULL)),
	slots_used(0),
	max_pipeline_size(1),
	exchange_key(true)
{
	recv_buff.reserve(global::C_MAX_SIZE*global::PIPELINE_SIZE);
	send_buff.reserve(global::S_MAX_SIZE*global::PIPELINE_SIZE);

	//look at encryption.h for key exchange protocol
	send_buff += Encryption.get_prime();
	Encryption.set_prime(send_buff);
	send_buff += Encryption.get_local_result();
	++send_pending;
}

client_buffer::~client_buffer()
{
	unregister_all();
}

void client_buffer::recv_buff_append(char * buff, const int & n_bytes)
{
	if(exchange_key){
		remote_result.append(buff, n_bytes);
		if(remote_result.size() == global::DH_KEY_SIZE){
			Encryption.set_remote_result(remote_result);
			recv_buff.clear();
			exchange_key = false;
		}
		if(remote_result.size() > global::DH_KEY_SIZE){
			LOGGER << " abusive, failed key negotation, too many bytes";
			DB_Blacklist.add(IP);
		}
		return;
	}

	Encryption.crypt_recv(buff, n_bytes);
	recv_buff.append(buff, n_bytes);
	post_recv();
}

bool client_buffer::empty()
{
	return Download.empty();
}

void client_buffer::post_recv()
{
	last_seen = time(NULL);

	while(!Pipeline.empty() && recv_buff.size() != 0){
		if(Pipeline.front().Mode == download::BINARY_MODE){
			//find out how many bytes are expected for this command
			std::vector<std::pair<char, int> >::iterator iter_cur, iter_end;
			iter_cur = Pipeline.front().expected.begin();
			iter_end = Pipeline.front().expected.end();
			while(iter_cur != iter_end){
				if(iter_cur->first == recv_buff[0]){
					break;
				}
				++iter_cur;
			}

			if(iter_cur == iter_end){
				LOGGER << "abusive " << IP << ", unexpected command " << (int)(unsigned char)recv_buff[0];
				DB_Blacklist.add(IP);
				return;
			}else{
				if(recv_buff.size() < iter_cur->second){
					//not enough bytes yet received to fulfill download's request
					if(Pipeline.front().Download.get() != NULL){
						Pipeline.front().Download->update_speed(socket, recv_buff.size() - bytes_seen);
					}
					bytes_seen = recv_buff.size();
					break;
				}

				if(Pipeline.front().Download.get() == NULL){
					//terminated download detected, discard response
					recv_buff.erase(0, iter_cur->second);
				}else{
					//pass response to download
					Pipeline.front().Download->response(socket, recv_buff.substr(0, iter_cur->second));
					if(Pipeline.front().Download.get() != NULL){
						Pipeline.front().Download->update_speed(socket, iter_cur->second - bytes_seen);
					}
					bytes_seen = 0;
					recv_buff.erase(0, iter_cur->second);
				}
				Pipeline.pop_front();
			}
		}else if(Pipeline.front().Mode == download::TEXT_MODE){
			int loc;
			if((loc = recv_buff.find_first_of('\0',0)) != std::string::npos){
				//pass response to download
				Pipeline.front().Download->response(socket, recv_buff.substr(0, loc));
				recv_buff.erase(0, loc+1);
				Pipeline.front().Download->update_speed(socket, loc);
				Pipeline.pop_front();
			}else{
				//whole message not yet received
				break;
			}
		}else{
			LOGGER << "invalid mode";
			exit(1);
		}
	}

	if(recv_buff.size() > global::PIPELINE_SIZE * global::C_MAX_SIZE){
		//server sent more than the maximum possible
		LOGGER << "abusive server (exceeded maximum buffer size) " << IP;
		DB_Blacklist.add(IP);
	}
}

void client_buffer::prepare_request()
{
	if(exchange_key){
		return;
	}

	if(Download.empty()){
		return;
	}

	/*
	If the pipeline size is zero that means the server satisfied all requests in
	the pipeline and that the pipeline needs to be bigger.

	If the pipeline is larger than one then we're overpipelining and the pipeline
	size should be decreased.
	*/
	if(Pipeline.size() == 0 && max_pipeline_size < global::PIPELINE_SIZE){
		++max_pipeline_size;
	}else if(Pipeline.size() > 1 && max_pipeline_size > 1){
		--max_pipeline_size;
	}

	//needed to stop infinite loop when all downloads waiting
	bool buffer_change = true;
	int initial_empty = send_buff.empty();
	while(Pipeline.size() < max_pipeline_size){
		if(rotate_downloads()){
			/*
			When rotate_downloads returns true it means that a full rotation of the
			ring buffer has been done. If the buffer has not been modified after one
			full rotation it means that all the downloads have no requests to make.
			*/
			if(!buffer_change){
				break;
			}
			buffer_change = false;
		}

		if((*Download_iter)->complete()){
			//download complete, don't attempt to get a request
			continue;
		}

		std::string request;
		pending_response PR;
		PR.Download = *Download_iter;
		PR.Mode = (*Download_iter)->request(socket, request, PR.expected, slots_used);

		//make sure slots_used value is within valid range
		assert(slots_used >= 0 || slots_used <= 255);

		Encryption.crypt_send(request);

		if(PR.Mode == download::NO_REQUEST){
			//no request to be made from the download
			continue;
		}else{
			send_buff += request;
			if(PR.Mode == download::BINARY_MODE){
				if(!PR.expected.empty()){
					//no response expected, don't add to pipeline
					Pipeline.push_back(PR);
				}
			}else if(PR.Mode == download::TEXT_MODE){
				//text mode always expects a response
				Pipeline.push_back(PR);
			}
			buffer_change = true;
		}
	}

	if(initial_empty && !send_buff.empty()){
		++send_pending;
	}
}

void client_buffer::register_download(locking_shared_ptr<download> new_download)
{
	if(Download.empty()){
		Download.push_back(new_download);
		Download_iter = Download.begin();
	}else{
		Download.push_back(new_download);
	}
}

bool client_buffer::rotate_downloads()
{
	if(!Download.empty()){
		++Download_iter;
	}
	if(Download_iter == Download.end()){
		Download_iter = Download.begin();
		return true;
	}
	return false;
}

void client_buffer::terminate_download(locking_shared_ptr<download> term_DL)
{
	/*
	If this download is in the Pipeline set the download pointer to NULL to
	indicate that incoming data for the download should be discarded.
	*/
	std::deque<pending_response>::iterator P_iter_cur, P_iter_end;
	P_iter_cur = Pipeline.begin();
	P_iter_end = Pipeline.end();
	while(P_iter_cur != P_iter_end){
		if(P_iter_cur->Download == term_DL){
			P_iter_cur->Download = locking_shared_ptr<download>();
		}
		++P_iter_cur;
	}

	//remove the download
	std::list<locking_shared_ptr<download> >::iterator D_iter_cur, D_iter_end;
	D_iter_cur = Download.begin();
	D_iter_end = Download.end();
	while(D_iter_cur != D_iter_end){
		if(*D_iter_cur == term_DL){
			D_iter_cur = Download.erase(D_iter_cur);
		}else{
			++D_iter_cur;
		}
	}

	//reset Download_iter because it may have been invalidated
	Download_iter = Download.begin();
}

void client_buffer::unregister_all()
{
	std::list<locking_shared_ptr<download> >::iterator iter_cur, iter_end;
	iter_cur = Download.begin();
	iter_end = Download.end();
	while(iter_cur != iter_end){
		(*iter_cur)->unregister_connection(socket);
		++iter_cur;
	}
}
