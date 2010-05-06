#include <net/net.hpp>

int main()
{
	std::set<net::endpoint> E = net::get_endpoint("localhost", "0");
	assert(!E.empty());
	net::ndgram N(*E.begin());
	assert(N.is_open());
	E = net::get_endpoint("localhost", N.local_port());
	assert(!E.empty());

	std::set<int> read, tmp_read, write, tmp_write;
	read.insert(N.socket());
	write.insert(N.socket());

	int send = 128, recv = 128;
	net::select select;
	while(send && recv){
		tmp_read = read;
		tmp_write = write;
		select(tmp_read, tmp_write);

		for(std::set<int>::iterator it_cur = tmp_read.begin(), it_end = tmp_read.end();
			it_cur != it_end; ++it_cur)
		{
			if(recv){
				net::buffer buf;
				boost::shared_ptr<net::endpoint> from;
				N.recv(buf, from);
				assert(buf.size() == 1);
				assert(buf == "x");
				assert(from);
				assert(N.is_open());
				--recv;
			}else{
				read.clear();
			}
		}

		for(std::set<int>::iterator it_cur = tmp_write.begin(), it_end = tmp_write.end();
			it_cur != it_end; ++it_cur)
		{
			if(send){
				net::buffer buf("x");
				N.send(buf, *E.begin());
				assert(buf.empty());
				assert(N.is_open());
				--send;
			}else{
				write.clear();
			}
		}
	}
}
