#ifndef H_CONNECTION
#define H_CONNECTION

//include
#include <boost/bind.hpp>
#include <boost/cstdint.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <convert.hpp>
#include <logger.hpp>
#include <network/network.hpp>

//standard
#include <fstream>
#include <map>
#include <sstream>
#include <string>

class connection : private boost::noncopyable
{
	static const int MTU = 16384;
public:
	connection(
		network::proactor & Proactor_in,
		network::connection_info & CI,
		const std::string & web_root_in
	);

private:
	const std::string web_root;
	network::proactor & Proactor;

	//if file requested path and index to end of data sent stored here
	boost::filesystem::path path;
	boost::uint64_t index;

	/*
	create_header:
		Return HTTP header.
	decode_chars:
		Replace HTML encoded chars with ASCII.
	encode_chars:
		Replace ASCII with HTML encoded chars.
	*/
	std::string create_header(const boost::uint64_t & content_length);
	void decode_chars(std::string & str);
	void encode_chars(std::string & str);

	//default call back
	void recv_call_back(network::connection_info & CI);

	/* Other Call Backs
	file_send_call_back:
		Incrementally sends file data.
	read_directory:
		Sends directory listing and disconnect.
	*/
	void file_send_call_back(network::connection_info & CI);
	void read_directory(network::connection_info & CI);
};
#endif
