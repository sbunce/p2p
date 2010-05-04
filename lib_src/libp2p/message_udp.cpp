#include "message_udp.hpp"

//BEGIN recv::find_node
message_udp::recv::find_node::find_node(handler func_in):
	func(func_in)
{

}

bool message_udp::recv::find_node::expect(const network::buffer & recv_buf)
{
	return recv_buf.size() == protocol_udp::find_node_size
		&& recv_buf[0] == protocol_udp::find_node;
}

bool message_udp::recv::find_node::recv(const network::buffer & recv_buf,
	const network::endpoint & endpoint)
{
	if(!expect(recv_buf)){
		return false;
	}
	network::buffer random(recv_buf.data()+1, 4);
	std::string remote_ID(convert::bin_to_hex(std::string(
		reinterpret_cast<const char *>(recv_buf.data())+5, 20)));
	std::string ID_to_find(convert::bin_to_hex(std::string(
		reinterpret_cast<const char *>(recv_buf.data())+25, 20)));
	func(endpoint, random, remote_ID, ID_to_find);
	return true;
}
//END recv::find_node

//BEGIN recv::host_list
message_udp::recv::host_list::host_list(
	handler func_in,
	const network::buffer & random_in
):
	func(func_in),
	random(random_in)
{

}

bool message_udp::recv::host_list::expect(const network::buffer & recv_buf)
{
	//check minimum size
	if(recv_buf.size() < protocol_udp::host_list_size || recv_buf.size() > 508){
		return false;
	}
	//check command
	if(recv_buf[0] != protocol_udp::host_list){
		return false;
	}
	//check random
	if(std::memcmp(recv_buf.data()+1, random.data(), 4) != 0){
		return false;
	}
	//verify address list makes sense
	bit_field BF(recv_buf.data()+25, 2, 16);
	unsigned offset = protocol_udp::host_list_size;
	for(unsigned x=0; x<16 && offset != recv_buf.size(); ++x){
		if(BF[x] == 0){
			//IPv4
			if(recv_buf.size() < offset + 7){
				return false;
			}
			if(recv_buf[offset + 6] > SHA1::bin_size * 8){
				return false;
			}
			offset += 7;
		}else{
			//IPv6
			if(recv_buf.size() < offset + 19){
				return false;
			}
			if(recv_buf[offset + 18] > SHA1::bin_size * 8){
				return false;
			}
			offset += 19;
		}
	}
	//check if longer than expected
	if(offset != recv_buf.size()){
		return false;
	}
	return true;
}

bool message_udp::recv::host_list::recv(const network::buffer & recv_buf,
	const network::endpoint & endpoint)
{
	if(!expect(recv_buf)){
		return false;
	}
	std::string remote_ID(convert::bin_to_hex(std::string(
		reinterpret_cast<const char *>(recv_buf.data())+5, 20)));
	bit_field BF(recv_buf.data()+25, 2, 16);
	std::list<std::pair<network::endpoint, unsigned char> > hosts;
	unsigned offset = protocol_udp::host_list_size;
	for(unsigned x=0; x<16 && offset != recv_buf.size(); ++x){
		if(BF[x] == 0){
			//IPv4
			boost::optional<network::endpoint> ep = network::bin_to_endpoint(
				recv_buf.str(offset, 4), recv_buf.str(offset + 4, 2));
			if(ep){
				hosts.push_back(std::make_pair(*ep, recv_buf[offset + 6]));
			}
			offset += 7;
		}else{
			//IPv6
			boost::optional<network::endpoint> ep = network::bin_to_endpoint(
				recv_buf.str(offset, 16), recv_buf.str(offset + 16, 2));
			if(ep){
				hosts.push_back(std::make_pair(*ep, recv_buf[offset + 18]));
			}
			offset += 19;
		}
	}
	func(endpoint, remote_ID, hosts);
	return true;
}
//END recv::host_list

//BEGIN recv::ping
message_udp::recv::ping::ping(
	handler func_in
):
	func(func_in)
{

}

bool message_udp::recv::ping::expect(const network::buffer & recv_buf)
{
	return recv_buf.size() == protocol_udp::ping_size
		&& recv_buf[0] == protocol_udp::ping;
}

bool message_udp::recv::ping::recv(const network::buffer & recv_buf,
	const network::endpoint & endpoint)
{
	if(!expect(recv_buf)){
		return false;
	}
	network::buffer random(recv_buf.data()+1, 4);
	std::string remote_ID(convert::bin_to_hex(std::string(
		reinterpret_cast<const char *>(recv_buf.data())+5, 20)));
	func(endpoint, random, remote_ID);
	return true;
}
//END recv::ping

//BEGIN recv::pong
message_udp::recv::pong::pong(
	handler func_in,
	const network::buffer & random_in
):
	func(func_in),
	random(random_in)
{

}

bool message_udp::recv::pong::expect(const network::buffer & recv_buf)
{
	if(recv_buf.size() != protocol_udp::pong_size){
		return false;
	}
	if(recv_buf[0] != protocol_udp::pong){
		return false;
	}
	return std::memcmp(recv_buf.data()+1, random.data(), 4) == 0;
}

bool message_udp::recv::pong::recv(const network::buffer & recv_buf,
	const network::endpoint & endpoint)
{
	if(!expect(recv_buf)){
		return false;
	}
	std::string remote_ID(convert::bin_to_hex(std::string(
		reinterpret_cast<const char *>(recv_buf.data())+5, 20)));
	func(endpoint, remote_ID);
	return true;
}
//END recv::pong

//BEGIN send::find_node
message_udp::send::find_node::find_node(const network::buffer & random,
	const std::string & local_ID, const std::string & ID_to_find)
{
	assert(random.size() == 4);
	assert(local_ID.size() == SHA1::hex_size);
	assert(ID_to_find.size() == SHA1::hex_size);
	buf.append(protocol_udp::find_node)
		.append(random)
		.append(convert::hex_to_bin(local_ID))
		.append(convert::hex_to_bin(ID_to_find));
}
//END send::find_node

//BEGIN send::host_list
message_udp::send::host_list::host_list(const network::buffer & random,
	const std::string & local_ID,
	const std::list<std::pair<network::endpoint, unsigned char> > & hosts)
{
	assert(random.size() == 4);
	assert(hosts.size() <= protocol_udp::host_list_elements);
	bit_field BF(16);
	unsigned cnt = 0;
	for(std::list<std::pair<network::endpoint, unsigned char> >::const_iterator
		it_cur = hosts.begin(), it_end = hosts.end(); it_cur != it_end; ++it_cur)
	{
		if(it_cur->first.version() == network::IPv4){
			BF[cnt] = 0;
		}else{
			BF[cnt] = 1;
		}
		++cnt;
	}
	buf.append(protocol_udp::host_list)
		.append(random)
		.append(convert::hex_to_bin(local_ID))
		.append(BF.get_buf());
	//append addresses
	for(std::list<std::pair<network::endpoint, unsigned char> >::const_iterator
		it_cur = hosts.begin(), it_end = hosts.end(); it_cur != it_end; ++it_cur)
	{
		buf.append(it_cur->first.IP_bin())
			.append(it_cur->first.port_bin())
			.append(it_cur->second);
	}
}
//END send::host_list

//BEGIN send::ping
message_udp::send::ping::ping(const network::buffer & random,
	const std::string & local_ID)
{
	assert(random.size() == 4);
	assert(local_ID.size() == SHA1::hex_size);
	buf.append(protocol_udp::ping)
		.append(random)
		.append(convert::hex_to_bin(local_ID));
}
//END send::ping

//BEGIN send::pong
message_udp::send::pong::pong(const network::buffer & random,
	const std::string & local_ID)
{
	assert(random.size() == 4);
	assert(local_ID.size() == SHA1::hex_size);
	buf.append(protocol_udp::pong)
		.append(random)
		.append(convert::hex_to_bin(local_ID));
}
//END send::ping
