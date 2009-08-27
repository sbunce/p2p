#include "reactor.hpp"

network::reactor::reactor():
	_incoming_connections(0),
	_max_incoming_connections(0),
	_outgoing_connections(0),
	_max_outgoing_connections(0)
{
	wrapper::start_networking();
}

network::reactor::~reactor()
{
	wrapper::stop_networking();
}

void network::reactor::add_job(boost::shared_ptr<network::sock> & S)
{
	boost::mutex::scoped_lock lock(job_mutex);
	job.push_back(S);
	job_cond.notify_one();
}

void network::reactor::connect(boost::shared_ptr<network::sock> & S)
{
	boost::mutex::scoped_lock lock(job_connect_mutex);
	if(!S->info->resolved()){
		S->failed_connect_flag = true;
		S->sock_error = FAILED_RESOLVE;
		add_job(S);
	}else{
		job_connect.push_back(S);
		trigger_selfpipe();
	}
}

unsigned network::reactor::connections()
{
	boost::mutex::scoped_lock lock(connections_mutex);
	return _incoming_connections + _outgoing_connections;
}

unsigned network::reactor::current_download_rate()
{
	return Rate_Limit.current_download_rate();
}

unsigned network::reactor::current_upload_rate()
{
	return Rate_Limit.current_upload_rate();
}

boost::shared_ptr<network::sock> network::reactor::get_finished_job()
{
	boost::mutex::scoped_lock lock(finished_job_mutex);
	if(finished_job.empty()){
		return boost::shared_ptr<sock>();
	}else{
		boost::shared_ptr<sock> S = finished_job.front();
		finished_job.pop_front();
		S->seen();
		return S;
	}
}

boost::shared_ptr<network::sock> network::reactor::get_job()
{
	boost::mutex::scoped_lock lock(job_mutex);
	while(job.empty()){
		job_cond.wait(job_mutex);
	}
	boost::shared_ptr<sock> S = job.front();
	job.pop_front();
	return S;
}

boost::shared_ptr<network::sock> network::reactor::get_job_connect()
{
	boost::mutex::scoped_lock lock(job_connect_mutex);
	if(job_connect.empty()){
		return boost::shared_ptr<sock>();
	}else{
		boost::shared_ptr<sock> S = job_connect.front();
		job_connect.pop_front();
		S->seen();
		return S;
	}
}

unsigned network::reactor::incoming_connections()
{
	boost::mutex::scoped_lock lock(connections_mutex);
	return _incoming_connections;
}

void network::reactor::incoming_decrement()
{
	boost::mutex::scoped_lock lock(connections_mutex);
	if(_incoming_connections == 0){
		LOGGER << "violated precondition";
		exit(1);
	}else{
		--_incoming_connections;
	}
}

void network::reactor::incoming_increment()
{
	boost::mutex::scoped_lock lock(connections_mutex);
	if(_incoming_connections >= _max_incoming_connections){
		LOGGER << "violated precondition";
		exit(1);
	}else{
		++_incoming_connections;
	}
}

bool network::reactor::incoming_limit_reached()
{
	boost::mutex::scoped_lock lock(connections_mutex);
	return _incoming_connections >= _max_incoming_connections;
}

void network::reactor::max_connections(const unsigned max_incoming_connections_in,
	const unsigned max_outgoing_connections_in)
{
	boost::mutex::scoped_lock lock(connections_mutex);
	if(max_incoming_connections_in + max_outgoing_connections_in
		> max_connections_supported())
	{
		LOGGER << "max_incoming + max_outgoing connections exceed implementation max";
		exit(1);
	}
	_max_incoming_connections = max_incoming_connections_in;
	_max_outgoing_connections = max_outgoing_connections_in;
}

unsigned network::reactor::max_download_rate()
{
	return Rate_Limit.max_download_rate();
}

void network::reactor::max_download_rate(const unsigned rate)
{
	Rate_Limit.max_download_rate(rate);
}

unsigned network::reactor::max_incoming_connections()
{
	boost::mutex::scoped_lock lock(connections_mutex);
	return _max_incoming_connections;
}

unsigned network::reactor::max_outgoing_connections()
{
	boost::mutex::scoped_lock lock(connections_mutex);
	return _max_outgoing_connections;
}

unsigned network::reactor::max_upload_rate()
{
	return Rate_Limit.max_upload_rate();
}

void network::reactor::max_upload_rate(const unsigned rate)
{
	Rate_Limit.max_upload_rate(rate);
}

unsigned network::reactor::outgoing_connections()
{
	boost::mutex::scoped_lock lock(connections_mutex);
	return _outgoing_connections;
}

void network::reactor::outgoing_decrement()
{
	boost::mutex::scoped_lock lock(connections_mutex);
	if(_outgoing_connections == 0){
		LOGGER << "violated precondition";
		exit(1);
	}else{
		--_outgoing_connections;
	}
}

void network::reactor::outgoing_increment()
{
	boost::mutex::scoped_lock lock(connections_mutex);
	if(_outgoing_connections >= _max_outgoing_connections){
		LOGGER << "violated precondition";
		exit(1);
	}else{
		++_outgoing_connections;
	}
}

bool network::reactor::outgoing_limit_reached()
{
	boost::mutex::scoped_lock lock(connections_mutex);
	return _outgoing_connections >= _max_outgoing_connections;
}

void network::reactor::put_job(boost::shared_ptr<network::sock> & S)
{
	if(S->disconnect_flag || S->failed_connect_flag){
		if(S->socket_FD != -1){
			if(S->direction == INCOMING){
				incoming_decrement();
			}else{
				outgoing_decrement();
			}
		}
	}else{
		boost::mutex::scoped_lock lock(finished_job_mutex);

		//after the first get() of this sock the connect job is done
		S->connect_flag = true;

		//reset other flags
		S->recv_flag = false;
		S->send_flag = false;

		finished_job.push_back(S);
		trigger_selfpipe();
	}
}
