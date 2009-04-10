#include "p2p_buffer.hpp"

p2p_buffer::p2p_buffer()
{

}

bool p2p_buffer::current_download(const std::string & hash, download_status & status)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::map<std::string, boost::shared_ptr<download> >::iterator
	iter = Running_Download.find(hash);
	if(iter == Running_Download.end()){
		return false;
	}else{
		iter->second->get_download_status(status);
		return true
	}
}

void p2p_buffer::current_downloads(std::vector<download_status> & status)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::map<std::string, boost::shared_ptr<download> >::iterator
	iter_cur = Running_Download.begin(),
	iter_end = Running_Download.end();
	while(iter_cur != iter_end){
		iter->second->get_download_status(status);
		++iter_cur;
	}
}

void p2p_buffer::current_uploads(std::vector<upload_status> & status)
{
	boost::mutex::scoped_lock lock(Mutex);
	LOGGER << "finish this";
	exit(1);
}

void p2p_buffer::erase(const int & socket_FD)
{
	boost::mutex::scoped_lock lock(Mutex);
	LOGGER << "finish this";
	exit(1);
}

bool p2p_buffer::is_downloading(const std::string & hash)
{
	boost::mutex::scoped_lock lock(Mutex);
	return Running_Download.find(hash) != Running_Download.end();
}
