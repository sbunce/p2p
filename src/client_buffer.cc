#include <iostream>

#include "client_buffer.h"
#include "conversion.h"

client_buffer::client_buffer(const std::string & server_IP_in, atomic<int> * send_pending_in)
{
	server_IP = server_IP_in;
	send_pending = send_pending_in;
	latest_requested = 0;
	recv_buff.reserve(global::BUFFER_SIZE);
	send_buff.reserve(global::C_CTRL_SIZE);
	Download_iter = Download.begin();
	ready = true;
	abuse = false;
	terminating = false;

	last_seen = time(0);
}

void client_buffer::add_download(const unsigned int & file_ID, download * new_download)
{
	download_holder DownloadHolder(file_ID, new_download);
	Download.push_back(DownloadHolder);
	new_download->reg_conn(server_IP);
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

void client_buffer::post_recv()
{
	if(recv_buff.size() == Download_iter->Download->bytes_expected()){
		Download_iter->Download->response(latest_requested, recv_buff);
		recv_buff.clear();
		ready = true;
	}

	if(recv_buff.size() > Download_iter->Download->bytes_expected()){
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
	if(ready && !terminating){
		rotate_downloads();
		latest_requested = Download_iter->Download->get_request();
		send_buff = global::P_SBL + conversion::encode_int(Download_iter->file_ID) + conversion::encode_int(latest_requested);
		++(*send_pending);
		ready = false;
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
	//if currently on this download then still expecting bytes
	if(hash == Download_iter->Download->get_hash()){
		if(ready){ //if not expecting any more bytes
			Download_iter->Download->unreg_conn(server_IP);
			Download_iter = Download.erase(Download_iter);
			terminating = false;
			return true;
		}
		else{ //expecting more bytes, let them finish before terminating
			terminating = true;
			return false;
		}
	}

	std::list<download_holder>::iterator iter_cur, iter_end;
	iter_cur = Download.begin();
	iter_end = Download.end();
	while(iter_cur != iter_end){
		if(hash == iter_cur->Download->get_hash()){
			iter_cur->Download->unreg_conn(server_IP);
			Download.erase(iter_cur);
			break;
		}
		++iter_cur;
	}

	return true;
}

void client_buffer::unregister_all()
{
	std::list<download_holder>::iterator iter_cur, iter_end;
	iter_cur = Download.begin();
	iter_end = Download.end();
	while(iter_cur != iter_end){
		iter_cur->Download->unreg_conn(server_IP);
		++iter_cur;
	}
}

