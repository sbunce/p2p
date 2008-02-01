#include <iostream>

#include "client_buffer.h"
#include "conversion.h"

client_buffer::client_buffer(int socket_in, std::string & server_IP_in, atomic<int> * send_pending_in)
{
	socket = socket_in;
	server_IP = server_IP_in;
	send_pending = send_pending_in;
	recv_buff.reserve(global::C_MAX_SIZE);
	send_buff.reserve(global::S_MAX_SIZE);
	Download_iter = Download.begin();
	ready = true;
	abuse = false;
	terminating = false;
	last_seen = time(0);
}

client_buffer::~client_buffer()
{
	unreg_all();
}

void client_buffer::add_download(download * new_download)
{
	Download.push_back(new_download);
}

const bool client_buffer::empty()
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

int client_buffer::post_recv()
{
	//make sure the server didn't respond out of turn, don't try to do Pipeline.front() on an empty Pipeline
	if(Pipeline.empty()){
		abuse = true;
		return 0;
	}

	while(recv_buff.size() >= Pipeline.front().first){
		Pipeline.front().second->response(socket, recv_buff.substr(0, Pipeline.front().first));
		recv_buff.erase(0, Pipeline.front().first);
		Pipeline.pop();
		ready = true;

		if(Pipeline.empty()){
			break;
		}
	}

	//if recv_buff larger than largest case the server is doing something naughty
	if(recv_buff.size() > global::PIPELINE_SIZE * global::C_MAX_SIZE){
		abuse = true;
	}

	last_seen = time(0);
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
	if(ready && !terminating){
		int count = 0;
		while(Pipeline.size() <= global::PIPELINE_SIZE && ++count <= global::PIPELINE_SIZE){
			//serve the next download in line
			rotate_downloads();

			//attempt to get a request
			if((*Download_iter)->request(socket, request)){
				send_buff += request;
				Pipeline.push(std::make_pair((*Download_iter)->bytes_expected(),*Download_iter));
				ready = false;
			}
			else{
				//rotate until a request is gotten or a full rotation has been done
				std::list<download *>::iterator Download_iter_temp = Download_iter;
				rotate_downloads();
				while(Download_iter != Download_iter_temp){
					if((*Download_iter)->request(socket, request)){
						send_buff += request;
						Pipeline.push(std::make_pair((*Download_iter)->bytes_expected(),*Download_iter));
						ready = false;
						break;
					}
					rotate_downloads();
				}
			}
		}

		if(send_buff.size() != 0){
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

const bool client_buffer::terminate_download(const std::string & hash)
{
	if(hash == (*Download_iter)->hash()){
		if(ready){ //if not expecting any more bytes
			(*Download_iter)->unreg_conn(socket);
			Download_iter = Download.erase(Download_iter);
			terminating = false;
			return true;
		}
		else{ //expecting more bytes, let them finish before terminating
			terminating = true;
			return false;
		}
	}

	std::list<download *>::iterator iter_cur, iter_end;
	iter_cur = Download.begin();
	iter_end = Download.end();
	while(iter_cur != iter_end){
		if(hash == (*iter_cur)->hash()){
			(*iter_cur)->unreg_conn(socket);
			Download.erase(iter_cur);
			break;
		}
		++iter_cur;
	}

	return true;
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

