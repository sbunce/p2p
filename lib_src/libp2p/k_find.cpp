#include "k_find.hpp"

void k_find::add_to_all(const net::endpoint & ep, const std::string & remote_ID)
{
	for(std::map<std::string, boost::shared_ptr<k_find_job> >::iterator
		it_cur = Find.begin(), it_end = Find.end(); it_cur != it_end; ++it_cur)
	{
		mpa::mpint dist = k_func::distance(remote_ID, it_cur->first);
		it_cur->second->add(ep, dist);
	}
}

void k_find::node(const std::string & ID,
	const std::multimap<mpa::mpint, net::endpoint> & hosts,
	const boost::function<void (const net::endpoint &)> & call_back)
{
	std::map<std::string, boost::shared_ptr<k_find_job> >::iterator it = Find.find(ID);
	if(it == Find.end()){
		//add new job
		boost::shared_ptr<k_find_job> job(new k_find_job(hosts));
		job->register_call_back(call_back, k_find_job::exact_match);
		Find.insert(std::make_pair(ID, job));
	}else{
		//register call back with existing job
		it->second->register_call_back(call_back, k_find_job::exact_match);
	}
}

void k_find::recv_host_list(const net::endpoint & from, const std::string & remote_ID,
	const std::list<net::endpoint> & hosts, const std::string & ID_to_find)
{
	std::map<std::string, boost::shared_ptr<k_find_job> >::iterator it = Find.find(ID_to_find);
	if(it != Find.end()){
		mpa::mpint dist = k_func::distance(remote_ID, ID_to_find);
		it->second->recv_host_list(from, hosts, dist);
	}
}

std::list<std::pair<net::endpoint, std::string> > k_find::send_find_node()
{
	std::list<std::pair<net::endpoint, std::string> > jobs;
	for(std::map<std::string, boost::shared_ptr<k_find_job> >::iterator
		it_cur = Find.begin(), it_end = Find.end(); it_cur != it_end; ++it_cur)
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

void k_find::set(const std::string & ID,
	const std::multimap<mpa::mpint, net::endpoint> & hosts,
	const boost::function<void (const net::endpoint &)> & call_back)
{
	std::map<std::string, boost::shared_ptr<k_find_job> >::iterator it = Find.find(ID);
	if(it == Find.end()){
		//add new job
		boost::shared_ptr<k_find_job> job(new k_find_job(hosts));
		job->register_call_back(call_back, k_find_job::any_will_do);
		Find.insert(std::make_pair(ID, job));
	}else{
		//register call back with existing job
		it->second->register_call_back(call_back, k_find_job::any_will_do);
	}
}

void k_find::tick()
{
	//check timeouts
	for(std::map<std::string, boost::shared_ptr<k_find_job> >::iterator
		it_cur = Find.begin(); it_cur != Find.end();)
	{
		if(it_cur->second->timeout()){
			Find.erase(it_cur++);
		}else{
			++it_cur;
		}
	}
}
