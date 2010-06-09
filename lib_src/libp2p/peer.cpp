#include "peer.hpp"

//BEGIN conn_element
peer::conn_element::conn_element(const net::endpoint & remote_ep_in):
	remote_ep(remote_ep_in),
	Download_Speed(new net::speed_calc()),
	Upload_Speed(new net::speed_calc()),
	reg_cnt(1)
{

}

peer::conn_element::conn_element(const conn_element & CE):
	remote_ep(CE.remote_ep),	
	unsent(CE.unsent),
	memoize(CE.memoize),
	Download_Speed(CE.Download_Speed),
	Upload_Speed(CE.Upload_Speed),
	reg_cnt(CE.reg_cnt)
{

}

void peer::conn_element::add(const net::endpoint & ep)
{
	if(ep == remote_ep){
		//don't need to tell remote host it has file it is downloading
		return;
	}
	std::pair<std::set<net::endpoint>::iterator, bool> p = memoize.insert(ep);
	if(p.second){
		//not already memoized
		unsent.push(ep);
	}
}

boost::optional<net::endpoint> peer::conn_element::get()
{
	if(unsent.empty()){
		return boost::optional<net::endpoint>();
	}else{
		net::endpoint ep = unsent.front();
		unsent.pop();
		return ep;
	}
}
//END conn_element

boost::shared_ptr<net::speed_calc> peer::download_speed_calc(const int connection_ID)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::map<int, conn_element>::iterator it = Conn.find(connection_ID);
	if(it == Conn.end()){
		return boost::shared_ptr<net::speed_calc>();
	}else{
		return it->second.Download_Speed;
	}
}

boost::optional<net::endpoint> peer::get(const int connection_ID)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::map<int, conn_element>::iterator it = Conn.find(connection_ID);
	if(it == Conn.end()){
		return boost::optional<net::endpoint>();
	}else{
		return it->second.get();
	}
}

std::list<p2p::transfer_info::host_element> peer::host_info()
{
	std::list<p2p::transfer_info::host_element> tmp;
	for(std::map<int, conn_element>::iterator it_cur = Conn.begin(),
		it_end = Conn.end(); it_cur != it_end; ++it_cur)
	{
		p2p::transfer_info::host_element HE;
		HE.IP = it_cur->second.remote_ep.IP();
		HE.port = it_cur->second.remote_ep.port();
		HE.download_speed = it_cur->second.Download_Speed->speed();
		HE.upload_speed = it_cur->second.Upload_Speed->speed();
		tmp.push_back(HE);
	}
	return tmp;
}

void peer::reg(const int connection_ID, const net::endpoint & ep)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::map<int, conn_element>::iterator it = Conn.find(connection_ID);
	if(it == Conn.end()){
		conn_element CE(ep);
		for(std::map<int, conn_element>::iterator it_cur = Conn.begin(),
			it_end = Conn.end(); it_cur != it_end; ++it_cur)
		{
			//add endpoints from all other subscribed hosts
			CE.add(it_cur->second.remote_ep);
			//add new endpoint to all other subscribed hosts
			it_cur->second.add(CE.remote_ep);
		}
		//add new conn
		Conn.insert(std::make_pair(connection_ID, CE));
	}else{
		++it->second.reg_cnt;
		assert(it->second.reg_cnt == 1 || it->second.reg_cnt == 2);
		it->second.add(ep);
	}
}

void peer::unreg(const int connection_ID)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::map<int, conn_element>::iterator it = Conn.find(connection_ID);
	assert(it != Conn.end());
	if(it->second.reg_cnt == 1){
		Conn.erase(it);
	}else{
		--it->second.reg_cnt;
		assert(it->second.reg_cnt == 1 || it->second.reg_cnt == 2);
	}
}

boost::shared_ptr<net::speed_calc> peer::upload_speed_calc(const int connection_ID)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::map<int, conn_element>::iterator it = Conn.find(connection_ID);
	if(it == Conn.end()){
		return boost::shared_ptr<net::speed_calc>();
	}else{
		return it->second.Upload_Speed;
	}
}
