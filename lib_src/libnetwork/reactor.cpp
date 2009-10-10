#include "reactor.hpp"

network::reactor::reactor():
	max_incoming(0),
	max_outgoing(0)
{
	wrapper::start_networking();
}

network::reactor::~reactor()
{
	wrapper::stop_networking();
}

boost::shared_ptr<network::sock> network::reactor::call_back_finished_job()
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

boost::shared_ptr<network::sock> network::reactor::call_back_get_job()
{
	boost::mutex::scoped_lock lock(schedule_job_mutex);
	while(schedule_job.empty()){
		schedule_job_cond.wait(schedule_job_mutex);
	}
	boost::shared_ptr<sock> S = schedule_job.front();
	schedule_job.pop_front();
	return S;
}

void network::reactor::call_back_return_job(boost::shared_ptr<network::sock> & S)
{
	if(S->disconnect_flag || S->failed_connect_flag){
		if(S->socket_FD != -1){
			connection_remove(S);
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

void network::reactor::call_back_schedule_job(boost::shared_ptr<network::sock> & S)
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(schedule_job_mutex);
	schedule_job.push_back(S);
	schedule_job_cond.notify_one();
	}//end lock scope

	/*
	Erase sock from force_call_back (if it exists) to make sure the force call
	back is only done once.
	*/
	{//begin lock scope
	boost::mutex::scoped_lock lock(connections_mutex);
	force_call_back.erase(S);
	}//end lock scope
}

void network::reactor::connection_add(boost::shared_ptr<sock> & S)
{
	if(S->Direction == sock::incoming){
		boost::mutex::scoped_lock lock(connections_mutex);
		std::pair<std::set<boost::shared_ptr<sock> >::iterator, bool>
			ret = incoming.insert(S);
		if(!ret.second){
			LOGGER; exit(1);
		}
	}else{
		boost::mutex::scoped_lock lock(connections_mutex);
		std::pair<std::set<boost::shared_ptr<sock> >::iterator, bool>
			ret = outgoing.insert(S);
		if(!ret.second){
			LOGGER; exit(1);
		}
	}
}

bool network::reactor::connection_incoming_limit()
{
	boost::mutex::scoped_lock lock(connections_mutex);
	return incoming.size() >= max_incoming;
}

bool network::reactor::connection_outgoing_limit()
{
	boost::mutex::scoped_lock lock(connections_mutex);
	return outgoing.size() >= max_outgoing;
}

void network::reactor::connection_remove(boost::shared_ptr<sock> & S)
{
	if(S->Direction == sock::incoming){
		boost::mutex::scoped_lock lock(connections_mutex);
		incoming.erase(S);
	}else{
		boost::mutex::scoped_lock lock(connections_mutex);
		outgoing.erase(S);
	}
}

unsigned network::reactor::connections()
{
	boost::mutex::scoped_lock lock(connections_mutex);
	return incoming.size() + outgoing.size();
}

unsigned network::reactor::download_rate()
{
	return Rate_Limit.download_rate();
}

boost::shared_ptr<network::sock> network::reactor::connect_job_get()
{
	boost::mutex::scoped_lock lock(connect_job_mutex);
	if(connect_job.empty()){
		return boost::shared_ptr<sock>();
	}else{
		boost::shared_ptr<sock> S = connect_job.front();
		connect_job.pop_front();
		S->seen();
		return S;
	}
}

bool network::reactor::force_check(boost::shared_ptr<network::sock> & S)
{
	boost::mutex::scoped_lock lock(connections_mutex);
	return force_call_back.find(S) != force_call_back.end();
}

bool network::reactor::force_pending()
{
	boost::mutex::scoped_lock lock(connections_mutex);
	return !force_call_back.empty();
}

unsigned network::reactor::incoming_connections()
{
	boost::mutex::scoped_lock lock(connections_mutex);
	return incoming.size();
}

bool network::reactor::is_duplicate(boost::shared_ptr<sock> & S)
{
	boost::mutex::scoped_lock lock(connections_mutex);
	for(std::set<boost::shared_ptr<sock> >::iterator iter_cur = incoming.begin(),
		iter_end = incoming.end(); iter_cur != iter_end; ++iter_cur)
	{
		if((*iter_cur)->IP == S->IP){
			return true;
		}
	}
	for(std::set<boost::shared_ptr<sock> >::iterator iter_cur = outgoing.begin(),
		iter_end = outgoing.end(); iter_cur != iter_end; ++iter_cur)
	{
		if((*iter_cur)->IP == S->IP){
			return true;
		}
	}
	return false;
}

void network::reactor::max_connections(const unsigned max_incoming_in,
	const unsigned max_outgoing_in)
{
	boost::mutex::scoped_lock lock(connections_mutex);
	if(max_incoming_in + max_outgoing_in > supported_connections()){
		LOGGER << "max_incoming + max_outgoing connections exceed implementation max";
		exit(1);
	}
	max_incoming = max_incoming_in;
	max_outgoing = max_outgoing_in;
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
	return max_incoming;
}

unsigned network::reactor::max_outgoing_connections()
{
	boost::mutex::scoped_lock lock(connections_mutex);
	return max_outgoing;
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
	return outgoing.size();
}

void network::reactor::schedule_connect(boost::shared_ptr<network::sock> & S)
{
	if(!S->info->resolved()){
		//DNS resolution failure, or invalid address
		S->failed_connect_flag = true;
		S->Error = sock::failed_resolve;
		call_back_schedule_job(S);
	}else{
		{//begin lock scope
		boost::mutex::scoped_lock lock(connect_job_mutex);
		connect_job.push_back(S);
		}//end lock scope
		trigger_selfpipe();
	}
}

void network::reactor::trigger_call_back(const std::string & IP)
{
	boost::mutex::scoped_lock lock(connections_mutex);
	bool force = false; //set to true if a force_call_back scheduled
	for(std::set<boost::shared_ptr<sock> >::iterator iter_cur = incoming.begin(),
		iter_end = incoming.end(); iter_cur != iter_end; ++iter_cur)
	{
		if((*iter_cur)->IP == IP){
			force_call_back.insert(*iter_cur);
			force = true;
		}
	}

	for(std::set<boost::shared_ptr<sock> >::iterator iter_cur = outgoing.begin(),
		iter_end = outgoing.end(); iter_cur != iter_end; ++iter_cur)
	{
		if((*iter_cur)->host == IP){
			force_call_back.insert(*iter_cur);
			force = true;
		}
	}

	if(force){
		trigger_selfpipe();
	}
}

unsigned network::reactor::upload_rate()
{
	return Rate_Limit.upload_rate();
}
