#include "client_buffer.hpp"

//BEGIN STATIC
boost::mutex client_buffer::Mutex;
std::map<int, boost::shared_ptr<client_buffer> > client_buffer::Client_Buffer;
std::set<locking_shared_ptr<download> > client_buffer::Unique_Download;
int client_buffer::send_pending(0);

bool client_buffer::add_connection(download_connection & DC)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::map<int, boost::shared_ptr<client_buffer> >::iterator iter_cur, iter_end;
	iter_cur = Client_Buffer.begin();
	iter_end = Client_Buffer.end();
	while(iter_cur != iter_end){
		if(DC.IP == iter_cur->second->IP){
			DC.socket = iter_cur->second->socket;             //set socket of the discovered client_buffer
			DC.Download->register_connection(DC);             //register connection with download
			iter_cur->second->register_download(DC.Download); //register download with client_buffer
			return true;
		}
		++iter_cur;
	}
	return false;
}

void client_buffer::add_download(locking_shared_ptr<download> & Download)
{
	boost::mutex::scoped_lock lock(Mutex);
	Unique_Download.insert(Download);
}

void client_buffer::current_downloads(std::vector<download_status> & info, std::string hash)
{
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
}

void client_buffer::erase(const int & socket_FD)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::map<int, boost::shared_ptr<client_buffer> >::iterator iter = Client_Buffer.find(socket_FD);
	if(iter != Client_Buffer.end()){
		if(!iter->second->send_buff.empty()){
			//send_buff contains data, decrement send_pending
			--send_pending;
		}
		Client_Buffer.erase(iter);
	}
}

bool client_buffer::is_downloading(locking_shared_ptr<download> & Download)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::set<locking_shared_ptr<download> >::iterator iter = Unique_Download.find(Download);
	return iter != Unique_Download.end();
}

bool client_buffer::is_downloading(const std::string & hash)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::set<locking_shared_ptr<download> >::iterator iter_cur, iter_end;
	iter_cur = Unique_Download.begin();
	iter_end = Unique_Download.end();
	while(iter_cur != iter_end){
		if((*iter_cur)->hash() == hash){
			return true;
		}
		++iter_cur;
	}
	return false;
}

void client_buffer::generate_requests()
{
	boost::mutex::scoped_lock lock(Mutex);
	std::map<int, boost::shared_ptr<client_buffer> >::iterator iter_cur, iter_end;
	iter_cur = Client_Buffer.begin();
	iter_end = Client_Buffer.end();
	while(iter_cur != iter_end){
		iter_cur->second->prepare_request();
		++iter_cur;
	}
}

void client_buffer::get_complete(std::vector<locking_shared_ptr<download> > & complete)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::set<locking_shared_ptr<download> >::iterator UD_iter_cur, UD_iter_end;
	UD_iter_cur = Unique_Download.begin();
	UD_iter_end = Unique_Download.end();
	while(UD_iter_cur != UD_iter_end){
		if((*UD_iter_cur)->complete()){
			complete.push_back(*UD_iter_cur);

			//remove complete download from all client_buffers
			std::map<int, boost::shared_ptr<client_buffer> >::iterator CB_iter_cur, CB_iter_end;
			CB_iter_cur = Client_Buffer.begin();
			CB_iter_end = Client_Buffer.end();
			while(CB_iter_cur != CB_iter_end){
				CB_iter_cur->second->terminate_download(*UD_iter_cur);
				++CB_iter_cur;
			}

			//remove from unique download
			Unique_Download.erase(UD_iter_cur++);
		}else{
			++UD_iter_cur;
		}
	}
}

void client_buffer::get_empty(std::vector<int> & disconnect)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::map<int, boost::shared_ptr<client_buffer> >::iterator iter_cur, iter_end;
	iter_cur = Client_Buffer.begin();
	iter_end = Client_Buffer.end();
	while(iter_cur != iter_end){
		if(iter_cur->second->empty()){
			disconnect.push_back(iter_cur->first);
			Client_Buffer.erase(iter_cur++);
		}else{
			++iter_cur;
		}
	}
}

int client_buffer::get_send_pending()
{
	boost::mutex::scoped_lock lock(Mutex);
	assert(send_pending >= 0);
	return send_pending;
}

bool client_buffer::get_send_buff(const int & socket_FD, const int & max_bytes, std::string & destination)
{
	boost::mutex::scoped_lock lock(Mutex);
	destination.clear();
	std::map<int, boost::shared_ptr<client_buffer> >::iterator iter = Client_Buffer.find(socket_FD);
	if(iter == Client_Buffer.end()){
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

void client_buffer::get_timed_out(std::vector<int> & timed_out)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::map<int, boost::shared_ptr<client_buffer> >::iterator iter_cur, iter_end;
	iter_cur = Client_Buffer.begin();
	iter_end = Client_Buffer.end();
	while(iter_cur != iter_end){
		if(time(NULL) - iter_cur->second->last_seen >= global::TIMEOUT){
			timed_out.push_back(iter_cur->first);
			Client_Buffer.erase(iter_cur++);
		}else{
			++iter_cur;
		}
	}
}

void client_buffer::new_connection(const download_connection & DC)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::set<locking_shared_ptr<download> >::iterator UD_iter = Unique_Download.find(DC.Download);
	if(UD_iter == Unique_Download.end()){
		//download cancelled, silently fail to add connection
		return;
	}else{
		//download running, check if client_buffer exists
		std::map<int, boost::shared_ptr<client_buffer> >::iterator CB_iter;
		CB_iter = Client_Buffer.find(DC.socket);
		if(CB_iter == Client_Buffer.end()){
			std::pair<std::map<int, boost::shared_ptr<client_buffer> >::iterator, bool> ret;
			ret = Client_Buffer.insert(std::make_pair(DC.socket, new client_buffer(DC.socket, DC.IP)));
			assert(ret.second);
			CB_iter = ret.first;
		}

		//add download to client_buffer
		CB_iter->second->register_download(DC.Download);
		DC.Download->register_connection(DC);
	}
}

void client_buffer::post_send(const int & socket_FD, const int & n_bytes)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::map<int, boost::shared_ptr<client_buffer> >::iterator iter = Client_Buffer.find(socket_FD);
	if(iter != Client_Buffer.end()){
		iter->second->send_buff.erase(0, n_bytes);
		if(n_bytes && iter->second->send_buff.empty()){
			//buffer went from non-empty to empty
			--send_pending;
		}
	}
}

void client_buffer::post_recv(const int & socket_FD, char * buff, const int & n_bytes)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::map<int, boost::shared_ptr<client_buffer> >::iterator iter = Client_Buffer.find(socket_FD);
	if(iter != Client_Buffer.end()){
		iter->second->recv_buff_append(buff, n_bytes);
		iter->second->recv_buff_process();
	}
}

void client_buffer::stop_download(const std::string & hash)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::set<locking_shared_ptr<download> >::iterator iter_cur, iter_end;
	iter_cur = Unique_Download.begin();
	iter_end = Unique_Download.end();
	while(iter_cur != iter_end){
		if(hash == (*iter_cur)->hash()){
			(*iter_cur)->stop();
			return;
		}
		++iter_cur;
	}
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
	recv_buff.reserve(global::C_MAX_SIZE * global::PIPELINE_SIZE);
	send_buff.reserve(global::S_MAX_SIZE * global::PIPELINE_SIZE);

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

bool client_buffer::empty()
{
	return Download.empty();
}

void client_buffer::prepare_request()
{
	if(exchange_key){
		return;
	}

	if(Download.empty()){
		return;
	}

	//see header documentation for max_pipeline_size
	if(Pipeline.size() == 0 && max_pipeline_size < global::PIPELINE_SIZE){
		++max_pipeline_size;
	}else if(Pipeline.size() > 1 && max_pipeline_size > 1){
		--max_pipeline_size;
	}

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
		assert(slots_used >= 0 && slots_used <= 255);

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
}

void client_buffer::recv_buff_process()
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
