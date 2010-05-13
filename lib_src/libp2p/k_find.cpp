#include "k_find.hpp"

void k_find::add_to_all(const net::endpoint & ep, const std::string & remote_ID)
{
	for(std::map<std::string, boost::shared_ptr<k_find_job> >::iterator
		it_cur = Jobs.begin(), it_end = Jobs.end(); it_cur != it_end; ++it_cur)
	{
		mpa::mpint dist = k_func::distance(remote_ID, it_cur->first);
		it_cur->second->add(dist, ep);
	}
}

void k_find::add_job(const std::string & ID_to_find,
	const std::multimap<mpa::mpint, net::endpoint> & hosts,
	const boost::function<void (const net::endpoint &, const std::string)> & call_back)
{
	std::map<std::string, boost::shared_ptr<k_find_job> >::iterator it = Jobs.find(ID_to_find);
	if(it == Jobs.end()){
		//create new job
		std::pair<std::map<std::string, boost::shared_ptr<k_find_job> >::iterator, bool>
			ret = Jobs.insert(std::make_pair(ID_to_find,
			boost::shared_ptr<k_find_job>(new k_find_job(boost::bind(call_back, _1, ID_to_find)))));
		assert(ret.second);
		ret.first->second->add_initial(hosts);
	}
}

void k_find::cancel_job(const std::string & ID_to_find)
{
	Jobs.erase(ID_to_find);
}

std::list<std::pair<net::endpoint, std::string> > k_find::find_node()
{
	std::list<std::pair<net::endpoint, std::string> > jobs;
	for(std::map<std::string, boost::shared_ptr<k_find_job> >::iterator
		it_cur = Jobs.begin(), it_end = Jobs.end(); it_cur != it_end; ++it_cur)
	{
		std::list<net::endpoint> tmp = it_cur->second->find_node();
		for(std::list<net::endpoint>::iterator t_it_cur = tmp.begin(),
			t_it_end = tmp.end(); t_it_cur != t_it_end; ++t_it_cur)
		{
			jobs.push_back(std::make_pair(*t_it_cur, it_cur->first));
		}
	}
	return jobs;
}

void k_find::recv_host_list(const net::endpoint & from, const std::string & remote_ID,
	const std::list<net::endpoint> & hosts, const std::string & ID_to_find)
{
	std::map<std::string, boost::shared_ptr<k_find_job> >::iterator it = Jobs.find(ID_to_find);
	if(it != Jobs.end()){
		mpa::mpint dist = k_func::distance(remote_ID, ID_to_find);
		it->second->recv_host_list(from, hosts, dist);
	}
}
