#include "client_buffer.h"

std::map<int, client_buffer> client_buffer::Client_Buffer;
std::set<download *> client_buffer::Unique_Download;
boost::mutex client_buffer::Mutex;
volatile int client_buffer::send_pending(0);

client_buffer::client_buffer()
{
	/*
	The client_buffer is used in a std::map. If std::map::[] is ever used to
	locate a client_buffer that doesn't exist the default ctor will be called and
	the program will be killed.
	*/
	logger::debug(LOGGER_P1,"improperly constructed client_buffer");
	exit(1);
}

client_buffer::client_buffer(
	const int & socket_in,
	const std::string & IP_in
):
	socket(socket_in),
	IP(IP_in),
	last_seen(time(NULL))
{
	recv_buff.reserve(global::C_MAX_SIZE*global::PIPELINE_SIZE);
	send_buff.reserve(global::S_MAX_SIZE*global::PIPELINE_SIZE);
}

client_buffer::~client_buffer()
{
	unregister_all();
}

bool client_buffer::empty()
{
	return Download.empty();
}

void client_buffer::post_recv(const int & n_bytes)
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
				//server sent unexpected command
				logger::debug(LOGGER_P1," abusive server ", IP, ", unexpected command ", (int)(unsigned char)recv_buff[0]);
				DB_blacklist::add(IP);
				return;
			}else{
				if(recv_buff.size() < iter_cur->second){
					//not enough bytes yet received to fulfill download's request
					break;
				}

				if(Pipeline.front().Download == NULL){
					//terminated download detected, discard response
					recv_buff.erase(0, iter_cur->second);
				}else{
					//pass response to download
					Pipeline.front().Download->response(socket, recv_buff.substr(0, iter_cur->second));
					recv_buff.erase(0, iter_cur->second);
					Pipeline.front().Download->update_speed(socket, iter_cur->second);
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
			logger::debug(LOGGER_P1,"invalid mode");
			exit(1);
		}
	}

	if(recv_buff.size() > global::PIPELINE_SIZE * global::C_MAX_SIZE){
		//server sent more than the maximum possible
		logger::debug(LOGGER_P1,"abusive server (exceeded maximum buffer size) ",IP);
		DB_blacklist::add(IP);
	}
}

void client_buffer::post_send()
{
	if(send_buff.empty()){
		--send_pending;
	}
}

void client_buffer::prepare_request()
{
	if(Download.empty()){
		return;
	}

	//needed to stop infinite loop when all downloads waiting
	bool buffer_change = true;
	int initial_empty = (send_buff.size() == 0);
	while(Pipeline.size() < global::PIPELINE_SIZE){
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
		PR.Mode = (*Download_iter)->request(socket, request, PR.expected);

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

	if(initial_empty && send_buff.size() != 0){
		++send_pending;
	}
}

void client_buffer::register_download(download * new_download)
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

void client_buffer::terminate_download(download * term_DL)
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
			P_iter_cur->Download = NULL;
		}
		++P_iter_cur;
	}

	//remove the download
	std::list<download *>::iterator D_iter_cur, D_iter_end;
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
	std::list<download *>::iterator iter_cur, iter_end;
	iter_cur = Download.begin();
	iter_end = Download.end();
	while(iter_cur != iter_end){
		(*iter_cur)->unregister_connection(socket);
		++iter_cur;
	}
}
