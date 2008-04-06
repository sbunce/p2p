//std
#include <iostream>

//custom
#include "conversion.h"

#include "client_buffer.h"

client_buffer::client_buffer(const int & socket_in, const std::string & server_IP_in,
volatile int * send_pending_in)
: server_IP(server_IP_in), socket(socket_in), send_pending(send_pending_in),
abuse(false), last_seen(time(0))
{
	recv_buff.reserve(global::C_MAX_SIZE);
	send_buff.reserve(global::S_MAX_SIZE);
}

client_buffer::~client_buffer()
{
	unreg_all();
}

void client_buffer::add_download(download * new_download)
{
	Download.push_back(new_download);
	Download_iter = Download.begin();
}

bool client_buffer::empty()
{
	return Download.empty();
}

const std::string & client_buffer::get_IP()
{
	return server_IP;
}

const time_t & client_buffer::get_last_seen()
{
	return last_seen;
}

void client_buffer::post_recv()
{
	last_seen = time(0);

	if(Pipeline.empty()){
		//server responded out of turn
		global::debug_message(global::ERROR,__FILE__,__FUNCTION__,"abusive server (responded when pipeline empty) ",server_IP);
		abuse = true;
		return;
	}

	//if recv_buff larger than largest case the server is doing something naughty
	if(recv_buff.size() > global::PIPELINE_SIZE * global::C_MAX_SIZE){
		global::debug_message(global::ERROR,__FILE__,__FUNCTION__,"abusive server (exceeded maximum buffer size) ",server_IP);
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
		}else if(iter_cur == iter_end){
			//command not found, server sent unexpected command
			global::debug_message(global::ERROR,__FILE__,__FUNCTION__,"abusive server (incorrect length of command) ", server_IP);
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
		--(*send_pending);
	}
}

void client_buffer::prepare_request()
{
	std::string request;
	int count = 0;
	if(!Download.empty()){
		int empty_to_nonempty = false;
		while(Pipeline.size() <= global::PIPELINE_SIZE && ++count <= global::PIPELINE_SIZE){
			rotate_downloads();
			pending_response PR;
			PR.Download = *Download_iter;
			if((*Download_iter)->request(socket, request, PR.expected)){
				if(send_buff.size() == 0){
					empty_to_nonempty = true;
				}
				send_buff += request;
				Pipeline.push_back(PR);
			}
		}

		//if the send_buff went from empty to nonempty signal client that a send is pending
		if(empty_to_nonempty){
			++(*send_pending);
		}
	}
}

void client_buffer::rotate_downloads()
{
	if(!Download.empty()){
		++Download_iter;
	}
	if(Download_iter == Download.end()){
		Download_iter = Download.begin();
	}
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

void client_buffer::unreg_all()
{
	std::list<download *>::iterator iter_cur, iter_end;
	iter_cur = Download.begin();
	iter_end = Download.end();
	while(iter_cur != iter_end){
		(*iter_cur)->unreg_conn(socket);
		++iter_cur;
	}
}
