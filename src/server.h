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
		std::string client_IP;
		std::string file_ID;
		std::string file_name;
		int file_size;
		int speed;
		int percent_complete;
	};

	server();
	/*
	get_upload_info - updates and returns upload information
	                  returns false if no uploads
	get_total_speed - returns the total speed of all uploads(in bytes per second)
	*/
	bool get_upload_info(std::vector<info_buffer> & uploadInfo);
	int get_total_speed();
	bool is_indexing();
	void start();

private:
	//used by Upload_Speed to track the progress of an upload
	class speed_element
	{
	public:
		//the gui will need this when displaying the upload
		std::string file_name;

		//used to idenfity what upload this is
		std::string client_IP;
		int file_ID;

		//these vectors are parallel and used for upload speed calculation
		std::deque<int> download_second; //second at which second_bytes were uploaded
		std::deque<int> second_bytes;    //bytes in the second

		//used to calculate percent complete
		int file_size;  //size of file
		int file_block; //what file_block was last requested
	};

	//used by calculate_speed() to track upload speeds
	std::list<speed_element> Upload_Speed;

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
	void calculate_speed(const int & socketfd, const int & file_ID, const int & file_block);
	void main_thread();
	bool new_conn(const int & listener);
	int prepare_file_block(std::map<int, std::string>::iterator & SB_iter, const int & socket_FD, const int & file_ID, const int & blockNumber);
	void process_request(const int & socketfd, char recvBuffer[], const int & nbytes);

	boost::mutex SB_mutex; //mutex for all usage of Send_Buff
	boost::mutex RB_mutex; //mutex for all usage of Recv_Buff
	boost::mutex US_mutex; //mutex for all usage of Upload_Speed

	//keeps track of shared files
	server_index Server_Index;
};
#endif
