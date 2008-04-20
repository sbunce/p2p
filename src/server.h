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

//used to prompt select() to return
#include <signal.h>

//contains error codes select() might return
#include <errno.h>

//custom
#include "conversion.h"
#include "DB_access.h"
#include "global.h"
#include "server_index.h"
#include "speed_calculator.h"

class server
{
public:
	server();


//DEBUG, all this junk should be stored in buffer
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

	/*
	get_upload_info - retrieves information on all current uploads
	                  returns true if upload_info updated
	get_total_speed - returns the total speed of all uploads (bytes per second)
	is_indexing     - returns true if the server is indexing files
	start           - starts the server
	stop            - stops the server (blocks until server shut down)
	*/
	bool get_upload_info(std::vector<info_buffer> & upload_info);
	int get_total_speed();
	bool is_indexing();
	void start();
	void stop();

private:


//new junk
	class buffer
	{
	public:
		buffer(std::string client_IP_in)
		: client_IP(client_IP_in)
		{
			send_buff.reserve(global::C_MAX_SIZE * global::PIPELINE_SIZE);
			recv_buff.reserve(global::S_MAX_SIZE * global::PIPELINE_SIZE);
		}

		std::string client_IP;
		std::string send_buff, recv_buff;

		/*
		Stores file ID's and hash ID's that have been assigned to the client. The
		char will store a number from 0-255.
		Note: this limits simultaneous DL's from a client to 256.
		*/
		std::map<char, std::string> ID;
	};

	std::map<int, buffer> Buffer;




	//holds data associated with a connection
	class send_buff_element
	{
	public:
		send_buff_element(std::string client_IP_in)
		{
			client_IP = client_IP_in;
			buff.reserve(global::C_MAX_SIZE * global::PIPELINE_SIZE);
		}

		std::string buff;
		std::string client_IP;
	};

	//main send/recv buffers associated with the socket number
	std::map<int, send_buff_element> Send_Buff;
	std::map<int, std::string> Recv_Buff;

	volatile bool stop_threads; //if true this will trigger thread termination
	volatile int threads;       //how many threads are currently running

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

	//used by calculate_speed() to track upload speeds
	std::list<speed_element_file> Upload_Speed;

	//holds the total upload speed
	unsigned int total_speed;

	//how many are connections currently established
	int connections;

	//select() related
	fd_set master_FDS;   //master file descriptor set
	fd_set read_FDS;     //set when socket can read without blocking
	fd_set write_FDS;    //set when socket can write without blocking
	int FD_max;          //holds the number of the maximum socket

	//counter for how many sends need to be done
	volatile int send_pending;

	//buffer for reading file blocks from the HDD
	char prepare_file_block_buff[global::P_BLOCK_SIZE - 1];

	/*
	disconnect         - disconnect client and remove socket from master set
	calculate_speed    - process the sendQueue and update Upload_Speed
	main_thread        - where the party is at
	new_conn           - sets up socket for client
	                   - returns true if connection suceeded
	prepare_file_block - prepares a response to a file block request
	process_request    - interprets the request from a client and buffers it
	*/
	void disconnect(const int & socketfd);
	void calculate_speed_file(std::string & client_IP, const unsigned int & file_ID, const unsigned int & file_block, const unsigned long & file_size, const std::string & file_path);
	void main_thread();
	bool new_conn(const int & listener);
	void prepare_file_block(std::map<int, send_buff_element>::iterator & SB_iter, const int & socket_FD, const unsigned int & file_ID, const unsigned int & block_number);
	void process_request(const int & socketfd, char recvBuffer[], const int & n_bytes);

	boost::mutex SB_mutex; //mutex for all usage of Send_Buff
	boost::mutex RB_mutex; //mutex for all usage of Recv_Buff
	boost::mutex US_mutex; //mutex for all usage of Upload_Speed

	conversion Conversion;
	DB_access DB_Access;
	server_index Server_Index;
	speed_calculator Speed_Calculator;
};
#endif
