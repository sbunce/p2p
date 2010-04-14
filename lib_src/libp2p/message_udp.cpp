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
	if(recv_buf.size() < protocol_udp::host_list_size || recv_buf.size() > 508){
		return false;
	}
	if(recv_buf[0] != protocol_udp::host_list){
		return false;
	}
	if(std::memcmp(recv_buf.data()+1, random.data(), 4) != 0){
		return false;
	}

	//verify length makes sense
	unsigned IPv4_cnt = recv_buf[25];
	if(IPv4_cnt > protocol_udp::bucket_size){
		return false;
	}
	unsigned tmp = recv_buf.size();
	tmp -= protocol_udp::host_list_size;
	if(tmp < IPv4_cnt * 7){
		return false;
	}
	tmp -= IPv4_cnt * 7;
	if(tmp % 19 != 0){
		return false;
	}
	unsigned IPv6_cnt = tmp / 19;
	if(IPv4_cnt + IPv6_cnt > protocol_udp::bucket_size){
		return false;
	}

	//verify bucket bytes
	int IPv4_start = protocol_udp::host_list_size;
	for(int RRN=0; RRN < IPv4_cnt; ++RRN){
		unsigned char bucket_byte = recv_buf[IPv4_start + RRN * 7 + 6];
		if(bucket_byte > protocol_udp::bucket_count && bucket_byte != 255){
			return false;
		}
	}
	int IPv6_start = IPv4_start + IPv4_cnt * 7;
	for(int RRN=0; IPv6_start + RRN * 19 < recv_buf.size(); ++RRN){
		unsigned char bucket_byte = recv_buf[IPv6_start + RRN * 19 + 18];
		if(bucket_byte > protocol_udp::bucket_count && bucket_byte != 255){
			return false;
		}
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

	unsigned IPv4_cnt = recv_buf[25];
	std::list<std::pair<network::endpoint, unsigned char> > hosts;

	//read IPv4 addresses
	int IPv4_start = protocol_udp::host_list_size;
	for(int RRN=0; RRN < IPv4_cnt; ++RRN){
		hosts.push_back(
			std::make_pair(
				network::endpoint(
					recv_buf.str(IPv4_start + RRN * 7, 4),
					recv_buf.str(IPv4_start + RRN * 7 + 4, 2),
					network::tcp
				),
				recv_buf[IPv4_start + RRN * 7 + 6]
			)
		);
	}
	//read IPv6 addresses
	int IPv6_start = IPv4_start + IPv4_cnt * 7;
	for(int RRN=0; IPv6_start + RRN * 19 < recv_buf.size(); ++RRN){
		hosts.push_back(
			std::make_pair(
				network::endpoint(
					recv_buf.str(IPv6_start + RRN * 19, 16),
					recv_buf.str(IPv6_start + RRN * 19 + 16, 2),
					network::tcp
				),
				recv_buf[IPv6_start + RRN * 19 + 18]
			)
		);
	}
	func(endpoint, remote_ID, hosts);
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
	assert(hosts.size() <= protocol_udp::bucket_size);

	//separate IPv4 and IPv6 endpoints
	std::list<std::pair<network::endpoint, unsigned char> > IPv4_endpoint, IPv6_endpoint;
	for(std::list<std::pair<network::endpoint, unsigned char> >::const_iterator
		iter_cur = hosts.begin(), iter_end = hosts.end(); iter_cur != iter_end; ++iter_cur)
	{
		if(iter_cur->first.version() == network::IPv4){
			IPv4_endpoint.push_back(*iter_cur);
		}else{
			IPv6_endpoint.push_back(*iter_cur);
		}
	}
	unsigned char IPv4_count = IPv4_endpoint.size();

	buf.append(protocol_udp::host_list)
		.append(random)
		.append(convert::hex_to_bin(local_ID))
		.append(IPv4_count);

	//append IPv4 addresses
	for(std::list<std::pair<network::endpoint, unsigned char> >::iterator
		iter_cur = IPv4_endpoint.begin(), iter_end = IPv4_endpoint.end();
		iter_cur != iter_end; ++iter_cur)
	{
		buf.append(iter_cur->first.IP_bin())
			.append(iter_cur->first.port_bin())
			.append(iter_cur->second);
	}

	//append IPv6 addresses
	for(std::list<std::pair<network::endpoint, unsigned char> >::iterator
		iter_cur = IPv6_endpoint.begin(), iter_end = IPv6_endpoint.end();
		iter_cur != iter_end; ++iter_cur)
	{
		buf.append(iter_cur->first.IP_bin())
			.append(iter_cur->first.port_bin())
			.append(iter_cur->second);
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
