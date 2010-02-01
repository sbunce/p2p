#ifndef H_NETWORK_SELECT
#define H_NETWORK_SELECT

//include
#include "system_include.hpp"

namespace network{
class select
{
public:
	select()
	{
		/*
		Create socket pair for self-pipe trick. The read socket is monitored by
		select() and the write socket is used to interrupt select().
		*/
		//setup listener
		std::set<network::endpoint> E = network::get_endpoint("localhost", "0", network::tcp);
		assert(!E.empty());
		network::listener L(*E.begin());
		assert(L.is_open());

		//connect read_only socket
		E = network::get_endpoint("localhost", L.port(), network::tcp);
		assert(!E.empty());
		sp_read = boost::shared_ptr<nstream>(new nstream(*E.begin()));
		assert(sp_read->is_open());

		//accept write_only connection
		sp_write = L.accept();
		assert(sp_write);
	}

	void interrupt()
	{
		buffer B("0");
		sp_write->send(B);
	}

	void operator () (std::set<int> & read, std::set<int> & write, unsigned timeout_ms = 0)
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
		for(std::set<int>::iterator iter_cur = read.begin(), iter_end = read.end();	
			iter_cur != iter_end; ++iter_cur)
		{
			FD_SET(*iter_cur, &read_FDS);
		}
		for(std::set<int>::iterator iter_cur = write.begin(), iter_end = write.end();	
			iter_cur != iter_end; ++iter_cur)
		{
			FD_SET(*iter_cur, &write_FDS);
		}

		//timeout
		timeval tv;
		if(timeout_ms >= 1000){
			tv.tv_sec = timeout_ms / 1000;
		}else{
			tv.tv_sec = 0;
		}
		tv.tv_usec = (timeout_ms % 1000) * 1000;

		//select
		int service = ::select(end_FD, &read_FDS, &write_FDS, NULL,
			timeout_ms == 0 ? NULL : &tv);
		if(service == -1){
			//ignore interrupt signal, profilers can cause this
			if(errno != EINTR){
				LOGGER << errno; exit(1);
			}
		}else if(service == 0){
			read.clear();
			write.clear();
		}else{
			read.erase(sp_read->socket());
			if(FD_ISSET(sp_read->socket(), &read_FDS)){
				network::buffer buf;
				sp_read->recv(buf);
			}
			//remove sockets that don't need attention
			for(std::set<int>::iterator iter_cur = read.begin(), iter_end = read.end();	
				iter_cur != iter_end;)
			{
				if(!FD_ISSET(*iter_cur, &read_FDS)){
					read.erase(iter_cur++);
				}else{
					++iter_cur;
				}
			}
			for(std::set<int>::iterator iter_cur = write.begin(), iter_end = write.end();	
				iter_cur != iter_end;)
			{
				if(!FD_ISSET(*iter_cur, &write_FDS)){
					write.erase(iter_cur++);
				}else{
					++iter_cur;
				}
			}
		}
	}
private:
	//self-pipe
	boost::shared_ptr<nstream> sp_read;
	boost::shared_ptr<nstream> sp_write;
};
}//end namespace network
#endif
