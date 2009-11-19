#include "send_queue.hpp"

bool send_queue::expected(const network::buffer & recv_buf, unsigned & size)
{

}

void send_queue::insert(boost::shared_ptr<slot::message> & M)
{
	Send.push_back(M);
}

boost::shared_ptr<slot::message> send_queue::send()
{
	if(Send.empty()){
		return boost::shared_ptr<slot::message>();
	}else{
		boost::shared_ptr<slot::message> M = Send.front();
		Send.pop_front();
		if(!M->expected_response.empty()){
			Sent.push_back(M);
		}
		return M;
	}
}
