#include <network/network.hpp>

//declared here for PIMPL
#include "reactor_select.hpp"

network::proactor::proactor(
	boost::function<void (sock & Sock)> connect_call_back_in,
	boost::function<void (sock & Sock)> disconnect_call_back_in,
	boost::function<void (sock & Sock)> failed_connect_call_back_in,
	const bool duplicates_allowed,
	const std::string & port
):
	Reactor(new reactor_select(duplicates_allowed, port)),
	connect_call_back(connect_call_back_in),
	disconnect_call_back(disconnect_call_back_in),
	failed_connect_call_back(failed_connect_call_back_in)
{
	//start reactor that proactor uses
	Reactor->start();

	//start threads for doing call backs
	for(int x=0; x<boost::thread::hardware_concurrency(); ++x){
		Workers.create_thread(boost::bind(&proactor::dispatch, this));
	}

	//start thread for DNS resolution
	Workers.create_thread(boost::bind(&proactor::resolve, this));
}

network::proactor::~proactor()
{
	Reactor->stop();
	Workers.interrupt_all();
	Workers.join_all();
}

void network::proactor::connect(const std::string & host, const std::string & port)
{
	boost::mutex::scoped_lock lock(resolve_job_mutex);
	resolve_job.push_back(resolve_job_element(host, port,
		resolve_job_element::new_connection));
	resolve_job_cond.notify_one();
}

unsigned network::proactor::connections()
{
	return Reactor->connections();
}

unsigned network::proactor::connections_supported()
{
	return Reactor->connections_supported();
}

unsigned network::proactor::download_rate()
{
	return Reactor->download_rate();
}

unsigned network::proactor::incoming_connections()
{
	return Reactor->incoming_connections();
}

unsigned network::proactor::max_incoming_connections()
{
	return max_incoming_connections();
}

unsigned network::proactor::outgoing_connections()
{
	return Reactor->outgoing_connections();
}

unsigned network::proactor::max_outgoing_connections()
{
	return Reactor->max_outgoing_connections();
}

void network::proactor::max_connections(const unsigned max_incoming_connections_in,
	const unsigned max_outgoing_connections_in)
{
	Reactor->max_connections(max_incoming_connections_in, max_outgoing_connections_in);
}

unsigned network::proactor::max_download_rate()
{
	return Reactor->max_download_rate();
}

unsigned network::proactor::max_upload_rate()
{
	return Reactor->max_upload_rate();
}

void network::proactor::max_download_rate(const unsigned rate)
{
	Reactor->max_download_rate(rate);
}

void network::proactor::max_upload_rate(const unsigned rate)
{
	Reactor->max_upload_rate(rate);
}

void network::proactor::dispatch()
{
	while(true){
		boost::shared_ptr<sock> S = Reactor->call_back_get_job();
		if(S->failed_connect_flag){
			failed_connect_call_back(*S);
		}else{
			if(!S->connect_flag){
				connect_call_back(*S);
			}
			if(!S->disconnect_flag && S->recv_flag){
				assert(S->recv_call_back);
				S->recv_call_back(*S);
			}
			if(!S->disconnect_flag && S->send_flag){
				assert(S->send_call_back);
				S->send_call_back(*S);
			}
			if(S->disconnect_flag){
				disconnect_call_back(*S);
			}
		}
		Reactor->call_back_return_job(S);
	}
}

void network::proactor::resolve()
{
	while(true){
		resolve_job_element job;
		{//begin lock scope
		boost::mutex::scoped_lock lock(resolve_job_mutex);
		while(resolve_job.empty()){
			resolve_job_cond.wait(resolve_job_mutex);
		}
		job = resolve_job.front();
		resolve_job.pop_front();
		}//end lock scope
		boost::shared_ptr<address_info> AI(new address_info(
			job.host.c_str(), job.port.c_str()));

		if(job.type == resolve_job_element::new_connection){
			boost::shared_ptr<sock> S(new sock(AI));
			Reactor->schedule_connect(S);
		}else if(job.type == resolve_job_element::force_call_back){
			if(AI->resolved()){
				Reactor->trigger_call_back(wrapper::get_IP(*AI));
			}
		}else{
			LOGGER << "unhandled case";
			exit(1);
		}
	}
}

void network::proactor::trigger_call_back(const std::string & host,
	const std::string & port)
{
	boost::mutex::scoped_lock lock(resolve_job_mutex);
	resolve_job.push_back(resolve_job_element(host, port,
		resolve_job_element::force_call_back));
	resolve_job_cond.notify_one();
}

unsigned network::proactor::upload_rate()
{
	return Reactor->upload_rate();
}
