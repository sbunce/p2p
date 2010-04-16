#include <network/network.hpp>

int main()
{
	network::start();

	std::set<network::endpoint> E = network::get_endpoint("localhost", "0", network::udp);
	assert(!E.empty());
	network::ndgram N(*E.begin());
	assert(N.is_open());
	E = network::get_endpoint("localhost", N.local_port(), network::udp);
	assert(!E.empty());

	std::set<int> read, tmp_read, write, tmp_write;
	read.insert(N.socket());
	write.insert(N.socket());

	int send = 128, recv = 128;
	network::select select;
	while(send && recv){
		tmp_read = read;
		tmp_write = write;
		select(tmp_read, tmp_write);

		for(std::set<int>::iterator iter_cur = tmp_read.begin(), iter_end = tmp_read.end();
			iter_cur != iter_end; ++iter_cur)
		{
			if(recv){
				network::buffer buf;
				boost::shared_ptr<network::endpoint> from;
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

		for(std::set<int>::iterator iter_cur = tmp_write.begin(), iter_end = tmp_write.end();
			iter_cur != iter_end; ++iter_cur)
		{
			if(send){
				network::buffer buf("x");
				N.send(buf, *E.begin());
				assert(buf.empty());
				assert(N.is_open());
				--send;
			}else{
				write.clear();
			}
		}
	}
	network::stop();
}
