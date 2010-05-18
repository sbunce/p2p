#include "k_find.hpp"

//BEGIN find_node_element
k_find::find_node_element::find_node_element(
	const boost::function<void (const net::endpoint &)> & call_back_in,
	boost::shared_ptr<k_find_job> find_job_in
):
	call_back(call_back_in),
	find_job(find_job_in)
{

}

k_find::find_node_element::find_node_element(const find_node_element & FNE):
	call_back(FNE.call_back),
	find_job(FNE.find_job)
{

}
//END find_node_element

//BEGIN find_set_element
k_find::find_set_element::find_set_element(
	const boost::function<void (const net::endpoint &)> & call_back_in,
	boost::shared_ptr<k_find_job> find_job_in
):
	call_back(call_back_in),
	find_job(find_job_in),
	time(std::time(NULL))
{

}

k_find::find_set_element::find_set_element(const find_set_element & FSE):
	call_back(FSE.call_back),
	find_job(FSE.find_job),
	time(FSE.time)
{

}

bool k_find::find_set_element::timeout()
{
	return std::time(NULL) - time > protocol_udp::find_set_timeout;
}
//END find_set_element

void k_find::add_to_all(const net::endpoint & ep, const std::string & remote_ID)
{
	for(std::map<std::string, find_node_element>::iterator
		it_cur = Find_Node.begin(), it_end = Find_Node.end(); it_cur != it_end; ++it_cur)
	{
		mpa::mpint dist = k_func::distance(remote_ID, it_cur->first);
		it_cur->second.find_job->add(dist, ep);
	}
	for(std::map<std::string, find_set_element>::iterator
		it_cur = Find_Set.begin(), it_end = Find_Set.end(); it_cur != it_end; ++it_cur)
	{
		mpa::mpint dist = k_func::distance(remote_ID, it_cur->first);
		it_cur->second.find_job->add(dist, ep);
	}
}

void k_find::find_node(const std::string & ID_to_find,
	const std::multimap<mpa::mpint, net::endpoint> & hosts,
	const boost::function<void (const net::endpoint &)> & call_back)
{
	std::map<std::string, find_node_element>::iterator
		it = Find_Node.find(ID_to_find);
	if(it == Find_Node.end()){
		std::pair<std::map<std::string, find_node_element>::iterator, bool>
			ret = Find_Node.insert(std::make_pair(ID_to_find, find_node_element(
			call_back, boost::shared_ptr<k_find_job>(new k_find_job(hosts)))));
	}
}

void k_find::find_node_cancel(const std::string & ID_to_find)
{
	Find_Node.erase(ID_to_find);
}

void k_find::find_set(const std::string & ID,
	const std::multimap<mpa::mpint, net::endpoint> & hosts,
	const boost::function<void (const net::endpoint &)> & call_back)
{
	std::map<std::string, find_set_element>::iterator
		FS_it = Find_Set.find(ID);
	if(FS_it == Find_Set.end()){
		std::map<std::string, find_node_element>::iterator
			FN_it = Find_Node.find(ID);
		if(FN_it == Find_Node.end()){
			Find_Set.insert(std::make_pair(ID, find_set_element(call_back,
				boost::shared_ptr<k_find_job>(new k_find_job(hosts)))));
		}else{
			Find_Set.insert(std::make_pair(ID, find_set_element(call_back,
				FN_it->second.find_job)));
		}
	}
}

void k_find::recv_host_list(const net::endpoint & from, const std::string & remote_ID,
	const std::list<net::endpoint> & hosts, const std::string & ID_to_find)
{
	{
	std::map<std::string, find_node_element>::iterator
		it = Find_Node.find(ID_to_find);
	if(it != Find_Node.end()){
		if(remote_ID == ID_to_find){
			//remote host claims to be node we're looking for
			it->second.call_back(from);
		}
		mpa::mpint dist = k_func::distance(remote_ID, ID_to_find);
		it->second.find_job->recv_host_list(from, hosts, dist);
	}
	}

	{
	std::map<std::string, find_set_element>::iterator
		it = Find_Set.find(ID_to_find);
	if(it != Find_Set.end()){
		it->second.call_back(from);
		mpa::mpint dist = k_func::distance(remote_ID, ID_to_find);
		it->second.find_job->recv_host_list(from, hosts, dist);
	}
	}
}

std::list<std::pair<net::endpoint, std::string> > k_find::send_find_node()
{
	std::list<std::pair<net::endpoint, std::string> > jobs;
	for(std::map<std::string, find_node_element>::iterator
		it_cur = Find_Node.begin(), it_end = Find_Node.end(); it_cur != it_end; ++it_cur)
	{
		std::list<net::endpoint> tmp = it_cur->second.find_job->find_node();
		for(std::list<net::endpoint>::iterator t_it_cur = tmp.begin(),
			t_it_end = tmp.end(); t_it_cur != t_it_end; ++t_it_cur)
		{
			jobs.push_back(std::make_pair(*t_it_cur, it_cur->first));
		}
	}
	for(std::map<std::string, find_set_element>::iterator
		it_cur = Find_Set.begin(), it_end = Find_Set.end(); it_cur != it_end; ++it_cur)
	{
		std::list<net::endpoint> tmp = it_cur->second.find_job->find_node();
		for(std::list<net::endpoint>::iterator t_it_cur = tmp.begin(),
			t_it_end = tmp.end(); t_it_cur != t_it_end; ++t_it_cur)
		{
			jobs.push_back(std::make_pair(*t_it_cur, it_cur->first));
		}
	}
	return jobs;
}

void k_find::tick()
{
	//check timeouts on find_set_elements
	for(std::map<std::string, find_set_element>::iterator
		it_cur = Find_Set.begin(); it_cur != Find_Set.end();)
	{
		if(it_cur->second.timeout()){
			Find_Set.erase(it_cur++);
		}else{
			++it_cur;
		}
	}
}
