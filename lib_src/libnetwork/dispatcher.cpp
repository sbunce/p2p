#include <network/network.hpp>

network::dispatcher::dispatcher(
	const boost::function<void (connection_info &)> & connect_call_back_in,
	const boost::function<void (connection_info &)> & disconnect_call_back_in
):
	connect_call_back(connect_call_back_in),
	disconnect_call_back(disconnect_call_back_in),
	running_jobs(0),
	stopped(false)
{
	for(int x=0; x<dispatcher_threads; ++x){
		workers.create_thread(boost::bind(&dispatcher::dispatch, this));
	}
}

network::dispatcher::~dispatcher()
{
	//wait for existing jobs to finish
	stop_join();

	//stop threads
	workers.interrupt_all();
	workers.join_all();
}

void network::dispatcher::connect(const boost::shared_ptr<connection_info> & CI)
{
	boost::mutex::scoped_lock lock(job_mutex);
	assert(!stopped);
	job_list.push_front(std::make_pair(CI->connection_ID, boost::bind(
		&dispatcher::connect_call_back_wrapper, this, CI)));
	job_cond.notify_one();
}

void network::dispatcher::disconnect(const boost::shared_ptr<connection_info> & CI)
{
	boost::mutex::scoped_lock lock(job_mutex);
	assert(!stopped);
	job_list.push_back(std::make_pair(CI->connection_ID, boost::bind(
		&dispatcher::disconnect_call_back_wrapper, this, CI)));
	job_cond.notify_one();
}

void network::dispatcher::recv(const boost::shared_ptr<connection_info> & CI,
	const boost::shared_ptr<buffer> & recv_buf)
{
	boost::mutex::scoped_lock lock(job_mutex);
	assert(!stopped);
	job_list.push_back(std::make_pair(CI->connection_ID, boost::bind(
		&dispatcher::recv_call_back_wrapper, this, CI, recv_buf)));
	job_cond.notify_one();
}

void network::dispatcher::send(const boost::shared_ptr<connection_info> & CI,
	const unsigned latest_send, const unsigned send_buf_size)
{
	boost::mutex::scoped_lock lock(job_mutex);
	assert(!stopped);
	job_list.push_back(std::make_pair(CI->connection_ID, boost::bind(
		&dispatcher::send_call_back_wrapper, this, CI, latest_send, send_buf_size)));
	job_cond.notify_one();
}

void network::dispatcher::start()
{
	boost::mutex::scoped_lock lock(job_mutex);
	stopped = false;
}

void network::dispatcher::stop_join()
{
	boost::mutex::scoped_lock lock(job_mutex);
	stopped = true;
	while(!job_list.empty() || running_jobs){
		join_cond.wait(job_mutex);
	}
	assert(memoize.empty());
	assert(job_list.empty());
}

void network::dispatcher::dispatch()
{
	std::pair<int, boost::function<void ()> > tmp;
	while(true){
		{//BEGIN lock scope
		boost::mutex::scoped_lock lock(job_mutex);
		while(true){
			for(std::list<std::pair<int, boost::function<void ()> > >::iterator
				it_cur = job_list.begin(), it_end = job_list.end(); it_cur != it_end;
				++it_cur)
			{
				std::pair<std::set<int>::iterator, bool>
					ret = memoize.insert(it_cur->first);
				if(ret.second){
					tmp = *it_cur;
					job_list.erase(it_cur);
					++running_jobs;
					goto run_job;
				}
			}
			job_cond.wait(job_mutex);
		}
		}//END lock scope
		run_job:
		tmp.second();
		{//begin lock scope
		boost::mutex::scoped_lock lock(job_mutex);
		memoize.erase(tmp.first);
		--running_jobs;
		}//end lock scope
		job_cond.notify_all();
		join_cond.notify_all();
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
