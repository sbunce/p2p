#include "k_find.hpp"

void k_find::add_local(const std::string & ID_to_find,
	const std::multimap<mpa::mpint, net::endpoint> & hosts,
	const boost::function<void (const net::endpoint &, const std::string &)> & call_back)
{
	//if ID exists add hosts
	for(std::list<boost::shared_ptr<k_find_job> >::iterator it_cur = Job.begin(),
		it_end = Job.end(); it_cur != it_end; ++it_cur)
	{
		if((*it_cur)->ID_to_find == ID_to_find){
			(*it_cur)->add_local(hosts);
			return;
		}
	}
	if(call_back){
		//ID doesn't exist, create new job
		Job.push_front(boost::shared_ptr<k_find_job>(new k_find_job(ID_to_find, call_back)));
		Job.front()->add_local(hosts);
	}
}

void k_find::cancel(const std::string & ID_to_find)
{
	for(std::list<boost::shared_ptr<k_find_job> >::iterator it_cur = Job.begin(),
		it_end = Job.end(); it_cur != it_end; ++it_cur)
	{
		if((*it_cur)->ID_to_find == ID_to_find){
			Job.erase(it_cur);
			return;
		}
	}
}

boost::optional<std::pair<net::endpoint, std::string> > k_find::find_node()
{
	if(!Job.empty()){
		//rotate until find_node job, or until complete rotation done
		boost::shared_ptr<k_find_job> start = Job.front();
		boost::optional<net::endpoint> ep;
		boost::optional<std::pair<net::endpoint, std::string> > info;
		do{
			//check if find_node needs to be sent
			ep = Job.front()->find_node();
			if(ep){
				info = boost::optional<std::pair<net::endpoint, std::string> >(
					std::make_pair(*ep, Job.front()->ID_to_find));
			}
			//rotate
			Job.push_back(Job.front());
			Job.pop_front();
			//return job if we have one
			if(info){
				return info;
			}
		}while(start != Job.front());
	}
	return boost::optional<std::pair<net::endpoint, std::string> >();
}

void k_find::recv_host_list(const net::endpoint & from, const std::string & remote_ID,
	const std::list<net::endpoint> & hosts, const std::string & ID_to_find)
{
	for(std::list<boost::shared_ptr<k_find_job> >::iterator it_cur = Job.begin(),
		it_end = Job.end(); it_cur != it_end; ++it_cur)
	{
		if((*it_cur)->ID_to_find == ID_to_find){
			if(remote_ID == ID_to_find){
				//remote host claims to be node we're looking for
				(*it_cur)->call_back(from, ID_to_find);
			}
			(*it_cur)->recv_host_list(from, remote_ID, hosts, ID_to_find);
			return;
		}
	}
}
