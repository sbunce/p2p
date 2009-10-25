#ifndef H_HTTP
#define H_HTTP

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

class http : private boost::noncopyable
{
	static const int MTU = 16384;
public:
	http(network::proactor & Proactor_in, const std::string & web_root_in);

	//initial call back
	void recv_call_back(network::connection_info & CI);

private:
	network::proactor & Proactor;
	const std::string web_root;

	//if file requested path and index to end of data sent stored here
	boost::filesystem::path path;
	boost::uint64_t index;

	/*
	create_header:
		Creates a HTTP header. Content length should be the size (bytes) of the
		document we're sending.
	decode_chars:
		Replace HTML encoded characters with ASCII.
	encode_chars:
		Replace special ASCII characters with HTML encoded characters.
	file_send_call_back:
		Send call back used to send a file. Refills the send_buf with data from
		the file when it gets low.
	read_directory:
		Appends directory listing to network::buffer.
	*/
	std::string create_header(const boost::uint64_t & content_length);
	void decode_chars(std::string & str);
	void encode_chars(std::string & str);
	void file_send_call_back(network::connection_info & CI);
	void read_directory(network::connection_info & CI);
};
#endif
