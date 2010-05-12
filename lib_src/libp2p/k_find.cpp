#include "k_find.hpp"

void k_find::add_to_all(const net::endpoint & ep, const std::string & remote_ID)
{
	for(std::map<std::string, boost::shared_ptr<k_find_job> >::iterator
		it_cur = Job.begin(), it_end = Job.end(); it_cur != it_end; ++it_cur)
	{
		mpa::mpint dist = k_func::distance(remote_ID, it_cur->first);
		it_cur->second->add(dist, ep);
	}
}

void k_find::add_job(const std::string & ID_to_find,
	const std::multimap<mpa::mpint, net::endpoint> & hosts,
	const boost::function<void (const net::endpoint &, const std::string)> & call_back)
{
	std::map<std::string, boost::shared_ptr<k_find_job> >::iterator it = Job.find(ID_to_find);
	if(it == Job.end()){
		//create new job
		std::pair<std::map<std::string, boost::shared_ptr<k_find_job> >::iterator, bool>
			ret = Job.insert(std::make_pair(ID_to_find,
			boost::shared_ptr<k_find_job>(new k_find_job(boost::bind(call_back, _1, ID_to_find)))));
		assert(ret.second);
		for(std::multimap<mpa::mpint, net::endpoint>::const_iterator
			it_cur = hosts.begin(), it_end = hosts.end(); it_cur != it_end; ++it_cur)
		{
			ret.first->second->add(it_cur->first, it_cur->second);
		}
	}else{
		//add hosts to existing job
		for(std::multimap<mpa::mpint, net::endpoint>::const_iterator
			it_cur = hosts.begin(), it_end = hosts.end(); it_cur != it_end; ++it_cur)
		{
			it->second->add(it_cur->first, it_cur->second);
		}
	}
}

void k_find::cancel_job(const std::string & ID_to_find)
{
	Job.erase(ID_to_find);
}

boost::optional<std::pair<net::endpoint, std::string> > k_find::find_node()
{
	for(std::map<std::string, boost::shared_ptr<k_find_job> >::iterator
		it_cur = Job.upper_bound(last_job), it_end = Job.end(); it_cur != it_end; ++it_cur)
	{
		boost::optional<net::endpoint> ep = it_cur->second->find_node();
		if(ep){
			return boost::optional<std::pair<net::endpoint, std::string> >(
				std::make_pair(*ep, it_cur->first));
		}
	}
	for(std::map<std::string, boost::shared_ptr<k_find_job> >::iterator
		it_cur = Job.begin(), it_end = Job.upper_bound(last_job); it_cur != it_end; ++it_cur)
	{
		boost::optional<net::endpoint> ep = it_cur->second->find_node();
		if(ep){
			return boost::optional<std::pair<net::endpoint, std::string> >(
				std::make_pair(*ep, it_cur->first));
		}
	}
	return boost::optional<std::pair<net::endpoint, std::string> >();
}

void k_find::recv_host_list(const net::endpoint & from, const std::string & remote_ID,
	const std::list<net::endpoint> & hosts, const std::string & ID_to_find)
{
	std::map<std::string, boost::shared_ptr<k_find_job> >::iterator it = Job.find(ID_to_find);
	if(it != Job.end()){
		mpa::mpint dist = k_func::distance(remote_ID, ID_to_find);
		it->second->recv_host_list(from, hosts, dist);
	}
}
