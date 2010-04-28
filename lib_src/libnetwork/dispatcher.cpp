#include <network/network.hpp>

network::dispatcher::dispatcher(
	const boost::function<void (connection_info &)> & connect_call_back_in,
	const boost::function<void (connection_info &)> & disconnect_call_back_in
):
	connect_call_back(connect_call_back_in),
	disconnect_call_back(disconnect_call_back_in),
	job_stop(false)
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

void network::dispatcher::send(const boost::shared_ptr<connection_info> & CI,
	const unsigned latest_send, const unsigned send_buf_size)
{
	boost::mutex::scoped_lock lock(job_mutex);
	job.push_back(std::make_pair(CI->connection_ID, boost::bind(
		&dispatcher::send_call_back_wrapper, this, CI, latest_send,
		send_buf_size)));
	job_cond.notify_one();
}

void network::dispatcher::start()
{
	boost::mutex::scoped_lock lock(start_stop_mutex);
	assert(!workers);
	workers = boost::shared_ptr<boost::thread_group>(new boost::thread_group);
	for(int x=0; x<dispatcher_threads; ++x){
		workers->create_thread(boost::bind(&dispatcher::dispatch, this));
	}
}

void network::dispatcher::stop()
{
	boost::mutex::scoped_lock lock(start_stop_mutex);

	//tell workers to terminate when jobs done
	{//BEGIN lock scope
	boost::mutex::scoped_lock lock(job_mutex);
	job_stop = true;
	}//END lock scope

	//have all workers check for jobs
	job_cond.notify_all();

	//wait for workers to terminate
	workers->join_all();

	//these should be empty if workers terminated properly
	assert(memoize.empty());
	assert(job.empty());

	//ready for next start()
	workers = boost::shared_ptr<boost::thread_group>();
}

void network::dispatcher::dispatch()
{
	std::pair<int, boost::function<void ()> > tmp;
	while(true){
		{//BEGIN lock scope
		boost::mutex::scoped_lock lock(job_mutex);
		while(true){
			for(std::list<std::pair<int, boost::function<void ()> > >::iterator
				it_cur = job.begin(), it_end = job.end(); it_cur != it_end;
				++it_cur)
			{
				std::pair<std::set<int>::iterator, bool>
					ret = memoize.insert(it_cur->first);
				if(ret.second){
					tmp = *it_cur;
					job.erase(it_cur);
					goto run_job;
				}
			}
			if(job_stop){
				//jobs finished, stop worker
				return;
			}
			job_cond.wait(job_mutex);
		}
		}//END lock scope
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
