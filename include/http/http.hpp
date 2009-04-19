/*
todo:
-have connection class to associate with socket number. This will allow partial
sends.
*/
#ifndef H_HTTP
#define H_HTTP

//boost
#include <boost/bind.hpp>
#include <boost/cstdint.hpp>
#include <boost/filesystem.hpp>

//include
#include <buffer.hpp>
#include <convert.hpp>
#include <logger.hpp>

//snet
#include <network.hpp>

//std
#include <fstream>
#include <map>
#include <sstream>
#include <string>

class http
{
public:
	http(
		const std::string & web_root_in,
		const unsigned max_upload_rate
	);

	//call backs for network
	void connect_call_back(int socket_FD, buffer & send_buff, network::direction Direction);
	void disconnect_call_back(int socket_FD);
	bool recv_call_back(int socket_FD, buffer & recv_buff, buffer & send_buff);
	bool send_call_back(int socket_FD, buffer & recv_buff, buffer & send_buff);

	/*
	monitor:
		Print speeds and how many connected.
	*/
	void monitor();

private:
	std::string web_root;

	class connection
	{
	public:
		connection():
			Type(UNDETERMINED),
			index(0)
		{}

		enum type{
			UNDETERMINED,
			INVALID,
			DIRECTORY,
			FILE,
			DONE
		};

		type Type;
		std::string get_path;
		boost::filesystem::path path;
		boost::uint64_t index;
	};

	//socket_FD mapped to connection info
	std::map<int, connection> Connection;

	/*
	determine_type:
		Sets connection::Type to what type of request was made.
	read:
		Appends new data on to send_buff if any exists.
		Precondition: determine_type must have been called.
	replace_encoded_chars:
		Replaces stuff like %20 with ' '.
	*/
	void determine_type(connection & Conn);
	void read(connection & Conn, buffer & send_buff);
	void replace_encoded_chars(std::string & get_path);

	network Network;
};
#endif
