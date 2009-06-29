#ifndef H_HTTP
#define H_HTTP

//custom
#include "../buffer.hpp"
#include "../network.hpp"

//include
#include <boost/bind.hpp>
#include <boost/cstdint.hpp>
#include <boost/filesystem.hpp>
#include <convert.hpp>
#include <logger.hpp>

//standard
#include <fstream>
#include <map>
#include <sstream>
#include <string>

class http : private boost::noncopyable
{
public:
	http(const std::string & web_root_in);

	//call backs for network
	void recv_call_back(network::socket & SD);
	void send_call_back(network::socket & SD);

private:
	enum type{
		UNDETERMINED,
		INVALID,
		DIRECTORY,
		FILE,
		DONE
	};

	type Type;
	std::string web_root;
	std::string get_path;
	boost::filesystem::path path;
	boost::uint64_t index;

	/*
	determine_type:
		Sets connection::Type to what type of request was made.
	read:
		Appends new data on to send_buff if any exists.
		Precondition: determine_type must have been called.
	replace_encoded_chars:
		Replaces stuff like %20 with ' '.
	*/
	void determine_type();
	void read(network::buffer & send_buff);
	void replace_encoded_chars();
};
#endif
