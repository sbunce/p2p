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
public:
	static const int MTU = 16384;

	http(const std::string & web_root_in);

	//call backs for network
	void recv_call_back(network::sock & S);
	void send_call_back(network::sock & S);

private:
	std::string web_root;

	enum{
		UNDETERMINED,
		INVALID,
		DIRECTORY,
		FILE,
		DONE
	} State;

	//path to file or directory requested
	boost::filesystem::path path;

	/*
	If file requested this will hold a index of where we are in the file. This is
	needed so we don't have to read the entire file in to memory before sending.
	*/
	boost::uint64_t index;

	/*
	create_header:
		Creates a HTTP header. Content length should be the size (bytes) of the
		document we're sending.
	read:
		Appends new data on to send_buff if any exists.
		Precondition: determine_type must have been called.
	replace_encoded_chars:
		Replaces stuff like %20 with ' '.
	*/
	std::string create_header(const unsigned content_length);
	void read(network::sock & S);
	void replace_encoded_chars(std::string & str);
};
#endif
