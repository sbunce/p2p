#include <net/select.hpp>

net::select::select()
{
	net::init::start();
	/*
	Create socket pair for self-pipe trick. The read socket is monitored by
	select() and the write socket is used to interrupt select().
	*/
	//setup listener
	std::set<net::endpoint> E = net::get_endpoint("localhost", "0");
	assert(!E.empty());
	net::listener L(*E.begin());
	assert(L.is_open());

	//connect read_only socket
	E = net::get_endpoint("localhost", L.port());
	assert(!E.empty());
	sp_read.reset(new nstream(*E.begin()));
	assert(sp_read->is_open());

	//accept write_only connection
	sp_write = L.accept();
	assert(sp_write);
}

net::select::~select()
{
	net::init::stop();
}

void net::select::interrupt()
{
	buffer B("0");
	sp_write->send(B);
}

void net::select::operator () (std::set<int> & read, std::set<int> & write,
	int timeout_ms)
{
	//monitor self-pipe (removed from read set later)
	assert(sp_read->is_open());
	read.insert(sp_read->socket());

	//end_FD
	int end_FD = 0;
	if(!read.empty() && *read.rbegin() >= end_FD){
		end_FD = *read.rbegin() + 1;
	}
	if(!write.empty() && *write.rbegin() >= end_FD){
		end_FD = *write.rbegin() + 1;
	}

	//fd_sets
	fd_set read_FDS, write_FDS;
	FD_ZERO(&read_FDS);
	FD_ZERO(&write_FDS);
	for(std::set<int>::iterator it_cur = read.begin(), it_end = read.end();	
		it_cur != it_end; ++it_cur)
	{
		FD_SET(*it_cur, &read_FDS);
	}
	for(std::set<int>::iterator it_cur = write.begin(), it_end = write.end();	
		it_cur != it_end; ++it_cur)
	{
		FD_SET(*it_cur, &write_FDS);
	}

	//timeout
	timeval tv;
	if(timeout_ms >= 0){
		if(timeout_ms >= 1000){
			tv.tv_sec = timeout_ms / 1000;
		}else{
			tv.tv_sec = 0;
		}
		tv.tv_usec = (timeout_ms % 1000) * 1000;
	}

	//select
	int service = ::select(end_FD, &read_FDS, &write_FDS, NULL,
		timeout_ms < 0 ? NULL : &tv);
	if(service == -1){
		//ignore interrupt signal, profilers can cause this
		if(errno != EINTR){
			LOG << strerror(errno);
			exit(1);
		}
	}else if(service == 0){
		read.clear();
		write.clear();
	}else{
		read.erase(sp_read->socket());
		if(FD_ISSET(sp_read->socket(), &read_FDS)){
			net::buffer buf;
			sp_read->recv(buf);
		}
		//remove sockets that don't need attention
		for(std::set<int>::iterator it_cur = read.begin(), it_end = read.end();	
			it_cur != it_end;)
		{
			if(!FD_ISSET(*it_cur, &read_FDS)){
				read.erase(it_cur++);
			}else{
				++it_cur;
			}
		}
		for(std::set<int>::iterator it_cur = write.begin(), it_end = write.end();	
			it_cur != it_end;)
		{
			if(!FD_ISSET(*it_cur, &write_FDS)){
				write.erase(it_cur++);
			}else{
				++it_cur;
			}
		}
	}
}
