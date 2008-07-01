#include "client_buffer.h"

std::map<int, client_buffer *> client_buffer::Client_Buffer;
std::set<download *> client_buffer::Unique_Download;
boost::mutex client_buffer::Mutex;
volatile int client_buffer::send_pending(0);

client_buffer::client_buffer(
	const int & socket_in,
	const std::string & IP_in
):
	socket(socket_in),
	IP(IP_in),
	abuse(false),
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

void client_buffer::post_recv()
{
	last_seen = time(NULL);
	if(Pipeline.empty()){
		//server responded out of turn
		logger::debug(LOGGER_P1," abusive server (responded when pipeline empty) ",IP);
		abuse = true;
		return;
	}

	if(recv_buff.size() > global::PIPELINE_SIZE * global::C_MAX_SIZE){
		//server sent more than the maximum possible
		logger::debug(LOGGER_P1," abusive server (exceeded maximum buffer size) ",IP);
		abuse = true;
		return;
	}

	while(true){
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

		if(iter_cur != iter_end){
			//not enough bytes yet received to fulfill download's request
			if(recv_buff.size() < iter_cur->second){
				break;
			}
		}else{
			//server sent unexpected command
			logger::debug(LOGGER_P1," abusive server ", IP, ", unexpected command ", (int)(unsigned char)recv_buff[0]);
			abuse = true;
			return;
		}

		if(Pipeline.front().Download == NULL){
			//terminated download detected, discard response
			recv_buff.erase(0, iter_cur->second);
		}else{
			//pass response to download
			Pipeline.front().Download->response(socket, recv_buff.substr(0, iter_cur->second));
			recv_buff.erase(0, iter_cur->second);
		}

		Pipeline.pop_front();
		if(Pipeline.empty() || recv_buff.size() == 0){
			break;
		}
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
		std::string request;
		if(rotate_downloads()){
			//this will only be checked when a full rotation of downloads has been done
			if(!buffer_change){
				break;
			}
			buffer_change = false;
		}

		pending_response PR;
		PR.Download = *Download_iter;
		if((*Download_iter)->request(socket, request, PR.expected)){
			send_buff += request;

			/*
			It is possible that the download expectes no response to the command.
			If it doesn't don't add the pending_response to the pipeline.
			*/
			if(!PR.expected.empty()){
				Pipeline.push_back(PR);
				buffer_change = true;
			}
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

	//reset Download_iter since it was invalidated
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
