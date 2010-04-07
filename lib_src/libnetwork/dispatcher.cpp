#include <network/network.hpp>

network::dispatcher::dispatcher(
	const boost::function<void (connection_info &)> & connect_call_back_in,
	const boost::function<void (connection_info &)> & disconnect_call_back_in
):
	connect_call_back(connect_call_back_in),
	disconnect_call_back(disconnect_call_back_in)
{

}

void network::dispatcher::connect(const boost::shared_ptr<connection_info> & CI)
{
	boost::mutex::scoped_lock lock(job_mutex);
	job.push_front(std::make_pair(CI->connection_ID, boost::bind(
		&dispatcher::connect_call_back_wrapper, this, CI)));
	job_cond.notify_one();
}

void network::dispatcher::disconnect(const boost::shared_ptr<connection_info> & CI)
{
	boost::mutex::scoped_lock lock(job_mutex);
	job.push_back(std::make_pair(CI->connection_ID, boost::bind(
		&dispatcher::disconnect_call_back_wrapper, this, CI)));
	job_cond.notify_one();
}

void network::dispatcher::recv(const boost::shared_ptr<connection_info> & CI,
	const boost::shared_ptr<buffer> & recv_buf)
{
	boost::mutex::scoped_lock lock(job_mutex);
	job.push_back(std::make_pair(CI->connection_ID, boost::bind(
		&dispatcher::recv_call_back_wrapper, this, CI, recv_buf)));
	job_cond.notify_one();
}

void network::dispatcher::send(const boost::shared_ptr<connection_info> & CI, const unsigned latest_send,
	const unsigned send_buf_size)
{
	boost::mutex::scoped_lock lock(job_mutex);
	job.push_back(std::make_pair(CI->connection_ID, boost::bind(
		&dispatcher::send_call_back_wrapper, this, CI, latest_send,
		send_buf_size)));
	job_cond.notify_one();
}

void network::dispatcher::start()
{
	for(int x=0; x<dispatcher_threads; ++x){
		workers.create_thread(boost::bind(&dispatcher::dispatch, this));
	}
}

void network::dispatcher::stop()
{
	workers.interrupt_all();
	workers.join_all();
	job.clear();
	memoize.clear();
}

void network::dispatcher::dispatch()
{
	std::pair<int, boost::function<void ()> > tmp;
	while(true){
		{//begin lock scope
		boost::mutex::scoped_lock lock(job_mutex);
		while(true){
			for(std::list<std::pair<int, boost::function<void ()> > >::iterator
				iter_cur = job.begin(), iter_end = job.end(); iter_cur != iter_end;
				++iter_cur)
			{
				std::pair<std::set<int>::iterator, bool>
					ret = memoize.insert(iter_cur->first);
				if(ret.second){
					tmp = *iter_cur;
					job.erase(iter_cur);
					goto run_job;
				}
			}
			job_cond.wait(job_mutex);
		}
		}//end lock scope
		run_job:

		//run call back
		tmp.second();

		{//begin lock scope
		boost::mutex::scoped_lock lock(job_mutex);
		memoize.erase(tmp.first);
		}//end lock scope
		job_cond.notify_all();
	}
}

void network::dispatcher::connect_call_back_wrapper(
	boost::shared_ptr<connection_info> CI)
{
	connect_call_back(*CI);
}

void network::dispatcher::disconnect_call_back_wrapper(
	boost::shared_ptr<connection_info> CI)
{
	disconnect_call_back(*CI);
}

void network::dispatcher::recv_call_back_wrapper(
	boost::shared_ptr<connection_info> CI, boost::shared_ptr<buffer> recv_buf)
{
	if(CI->recv_call_back){
		CI->recv_buf.append(*recv_buf);
		CI->latest_recv = recv_buf->size();
		CI->recv_call_back(*CI);
	}
}

void network::dispatcher::send_call_back_wrapper(boost::shared_ptr<connection_info> CI,
	const unsigned latest_send, const int send_buf_size)
{
	CI->latest_send = latest_send;
	CI->send_buf_size = send_buf_size;
	if(CI->send_call_back){
		CI->send_call_back(*CI);
	}
}
