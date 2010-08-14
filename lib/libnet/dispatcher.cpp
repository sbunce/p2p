#include <net/dispatcher.hpp>

net::dispatcher::dispatcher(
	const boost::function<void (connection_info &)> & connect_call_back_in,
	const boost::function<void (connection_info &)> & disconnect_call_back_in
):
	connect_call_back(connect_call_back_in),
	disconnect_call_back(disconnect_call_back_in),
	Thread_Pool(4)
{

}

net::dispatcher::~dispatcher()
{

}

void net::dispatcher::connect(const boost::shared_ptr<connection_info> & CI)
{
	{//BEGIN lock scope
	boost::mutex::scoped_lock lock(Mutex);
	Jobs.push_front(std::make_pair(CI->connection_ID, boost::bind(
		&dispatcher::connect_call_back_wrapper, this, CI)));
	}//END lock scope
	dispatch();
}

void net::dispatcher::disconnect(const boost::shared_ptr<connection_info> & CI)
{
	{//BEGIN lock scope
	boost::mutex::scoped_lock lock(Mutex);
	Jobs.push_back(std::make_pair(CI->connection_ID, boost::bind(
		&dispatcher::disconnect_call_back_wrapper, this, CI)));
	}//END lock scope
	dispatch();
}

void net::dispatcher::dispatch()
{
	boost::mutex::scoped_lock lock(Mutex);
	//schedule all jobs that can be scheduled
	for(std::list<std::pair<int, boost::function<void ()> > >::iterator
		it_cur = Jobs.begin(); it_cur != Jobs.end();)
	{
		std::pair<std::set<int>::iterator, bool> ret = Memoize.insert(it_cur->first);
		if(ret.second){
			Thread_Pool.enqueue(it_cur->second);
			it_cur = Jobs.erase(it_cur);
		}else{
			++it_cur;
		}
	}
}

void net::dispatcher::connect_call_back_wrapper(
	boost::shared_ptr<connection_info> CI)
{
	connect_call_back(*CI);
	{//begin lock scope
	boost::mutex::scoped_lock lock(Mutex);
	Memoize.erase(CI->connection_ID);
	}//end lock scope
	dispatch();
}

void net::dispatcher::disconnect_call_back_wrapper(
	boost::shared_ptr<connection_info> CI)
{
	disconnect_call_back(*CI);
	{//begin lock scope
	boost::mutex::scoped_lock lock(Mutex);
	Memoize.erase(CI->connection_ID);
	}//end lock scope
	dispatch();
}

void net::dispatcher::join()
{
	Thread_Pool.join();
}

void net::dispatcher::recv(const boost::shared_ptr<connection_info> & CI,
	const boost::shared_ptr<buffer> & recv_buf)
{
	{//BEGIN lock scope
	boost::mutex::scoped_lock lock(Mutex);
	Jobs.push_back(std::make_pair(CI->connection_ID, boost::bind(
		&dispatcher::recv_call_back_wrapper, this, CI, recv_buf)));
	}//END lock scope
	dispatch();
}

void net::dispatcher::recv_call_back_wrapper(
	boost::shared_ptr<connection_info> CI, boost::shared_ptr<buffer> recv_buf)
{
	if(CI->recv_call_back){
		CI->recv_buf.append(*recv_buf);
		CI->latest_recv = recv_buf->size();
		CI->recv_call_back(*CI);
	}
	{//begin lock scope
	boost::mutex::scoped_lock lock(Mutex);
	Memoize.erase(CI->connection_ID);
	}//end lock scope
	dispatch();
}

void net::dispatcher::send(const boost::shared_ptr<connection_info> & CI,
	const unsigned latest_send, const unsigned send_buf_size)
{
	{//BEGIN lock scope
	boost::mutex::scoped_lock lock(Mutex);
	Jobs.push_back(std::make_pair(CI->connection_ID, boost::bind(
		&dispatcher::send_call_back_wrapper, this, CI, latest_send, send_buf_size)));
	}//END lock scope
	dispatch();
}

void net::dispatcher::send_call_back_wrapper(boost::shared_ptr<connection_info> CI,
	const unsigned latest_send, const int send_buf_size)
{
	CI->latest_send = latest_send;
	CI->send_buf_size = send_buf_size;
	if(CI->send_call_back){
		CI->send_call_back(*CI);
	}
	{//begin lock scope
	boost::mutex::scoped_lock lock(Mutex);
	Memoize.erase(CI->connection_ID);
	}//end lock scope
	dispatch();
}
