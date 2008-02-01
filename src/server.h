#ifndef H_SERVER
#define H_SERVER

//boost
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>

//std
#include <deque>
#include <list>
#include <map>
#include <string>
#include <vector>

//networking
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

//contains enum type for error codes select() might return
#include <errno.h>

#include "global.h"
#include "server_index.h"

class server
{
public:
	//used to pass information to user interface
	class info_buffer
	{
	public:
		info_buffer(std::string client_IP_in, std::string file_ID_in, std::string file_name_in,
			unsigned long file_size_in, int speed_in, int percent_complete_in)
		{
			client_IP = client_IP_in;
			file_ID = file_ID_in;
			file_name = file_name_in;
			file_size = file_size_in;
			speed = speed_in;
			percent_complete = percent_complete_in;
		}

		std::string client_IP;
		std::string file_ID;
		std::string file_name;
		unsigned long file_size;
		int speed;
		int percent_complete;
	};

	server();
	/*
	get_upload_info - updates and returns upload information
	                  returns false if no uploads
	get_total_speed - returns the total speed of all uploads(in bytes per second)
	*/
	bool get_upload_info(std::vector<info_buffer> & upload_info);
	int get_total_speed();
	bool is_indexing();
	void start();

private:
	//used by Upload_Speed to track the progress of an upload
	class speed_element_file
	{
	public:
		speed_element_file(std::string file_name_in, std::string client_IP_in,
			unsigned int file_ID_in, unsigned long file_size_in, unsigned long file_block_in)
		{
			file_name = file_name_in;
			client_IP = client_IP_in;
			file_ID = file_ID_in;
			file_size = file_size_in;
			file_block = file_block_in;
		}

		//the gui will need this when displaying the upload
		std::string file_name;

		//used to idenfity what upload this is
		std::string client_IP;
		unsigned int file_ID;

		//std::pair<second, bytes in second>
		std::deque<std::pair<int, int> > Speed;

		//used to calculate percent complete
		unsigned long file_size;  //size of file
		unsigned long file_block; //what file_block was last requested
	};

//DEBUG, if keeping track of more than one type of upload then virtualize the speed elements

	//used by calculate_speed() to track upload speeds
	std::list<speed_element_file> Upload_Speed;

	/*
	Stores pending responses. The partial send buffers can be accessed by socket
	number with Send_Buff[socketNumber];
	*/
	std::map<int, std::string> Send_Buff;

	/*
	Stores partial requests. The partial requests can be accessed by socket number
	with Recv_Buff[socketNumber];
	*/
	std::map<int, std::string> Recv_Buff;

	//how many are connections currently established
	int connections;

	//select() related
	fd_set master_FDS;   //master file descriptor set
	fd_set read_FDS;     //set when socket can read without blocking
	fd_set write_FDS;    //set when socket can write without blocking
	int FD_max;          //holds the number of the maximum socket

	/*
	This is a count of how many sends there are waiting. This exists for the
	purpose of having an alternate select() call that doesn't involve write_FDS
	because using write_FDS hogs CPU. However when there is data to write it is
	proper to use write_FDS.
	*/
	int send_pending;

	/*
	disconnect          - disconnect client and remove socket from master set
	calculate_speed     - process the sendQueue and update Upload_Speed
	main_thread         - where the party is at
	new_conn            - sets up socket for client
	                    - returns true if connection suceeded
	prepare_send_buffer - prepares a response to a file block request
	process_request     - interprets the request from a client and buffers it
	sendBlock           - sends a file_block to a client
	stateTick           - do pending actions
	*/
	void disconnect(const int & socketfd);
	void calculate_speed_file(const int & socket_FD, const int & file_ID, const int & file_block, const unsigned long & file_size, const std::string & file_path);
	void main_thread();
	bool new_conn(const int & listener);
	int prepare_file_block(std::map<int, std::string>::iterator & SB_iter, const int & socket_FD, const int & file_ID, const int & block_number);
	void process_request(const int & socketfd, char recvBuffer[], const int & n_bytes);

	boost::mutex SB_mutex; //mutex for all usage of Send_Buff
	boost::mutex RB_mutex; //mutex for all usage of Recv_Buff
	boost::mutex US_mutex; //mutex for all usage of Upload_Speed

	//keeps track of shared files
	server_index Server_Index;
};
#endif
