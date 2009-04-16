/*
todo:
-have connection class to associate with socket number. This will allow partial
sends.
*/
#ifndef H_HTTP
#define H_HTTP

//boost
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>

//include
#include <buffer.hpp>
#include <convert.hpp>
#include <logger.hpp>

//snet
#include <network.hpp>

//std
#include <fstream>
#include <string>

class http
{
public:
	http();

	//call backs for network
	void connect_call_back(int socket_FD, buffer & recv_buff, buffer & send_buff,
		network::direction Direction);
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

	/*
	get_process:
		Processes get request and puts response in send_buff. This function
		expects get_command to be of the form GET <path>.
	*/
	void process_get(std::string get_command, buffer & send_buff);

	network Network;
};
#endif
