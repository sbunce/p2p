//THREADSAFE, THREAD-SPAWNING
#ifndef H_NETWORKING
#define H_NETWORKING

//boost
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

//include
#include <atomic_int.hpp>
#include <buffer.hpp>
#include <logger.hpp>
#include <rate_limit.hpp>
#include <reactor.hpp>
#include <reactor_select.hpp>
#include <speed_calculator.hpp>

//networking
#ifdef WIN32
	#define MSG_NOSIGNAL 0  //disable SIGPIPE on send()
	#include <winsock.h>
#else
	#include <arpa/inet.h>
	#include <fcntl.h>
	#include <netinet/in.h>
	#include <sys/socket.h>
	#include <unistd.h>
#endif

//std
#include <cstdlib>
#include <limits>
#include <map>
#include <queue>
#include <vector>

class network : private boost::noncopyable
{
public:
	typedef reactor::socket_data::socket_data_visible socket_data;

	/* ctor parameters
	boost::bind is the nicest way to do a callback to the member function of a
	specific object. ex:
		boost::bind(&http::connect_call_back, &HTTP, _1, _2, _3)

	connect_call_back:
		Called when server connected to. The direction indicates whether we
		established connection (direction OUTGOING), or if someone established a
		connection to us (direction INCOMING). The send_buff is to this function
		in case something needs to be sent after connect.
	recv_call_back:
		Called after data received (received data in recv_buff). A return value of 
		true will keep the server connected, but false will cause it to be
		disconnected.
	send_call_back:
		Called after data sent. The send_buff will contain data that hasn't yet
		been sent. A return value of true will keep the server connected, but
		false will cause it to be disconnected.
	disconnect_call-back:
		Called when a connected socket disconnects.
	connect_time_out:
		Seconds before connection attempt times out.
	socket_time_out:
		Seconds before socket with no I/O times out.
	listen_port:
		Port to listen on for incoming connections. The default (if no parameter
		specified) is to not listen for incoming connections.
	*/
	network(
		boost::function<void (socket_data &)> connect_call_back_in,
		boost::function<void (socket_data &)> disconnect_call_back_in,
		const int port = -1
	):
		connect_call_back(connect_call_back_in),
		disconnect_call_back(disconnect_call_back_in)
	{
		#ifdef WIN32
		WORD wsock_ver = MAKEWORD(2,2);
		WSADATA wsock_data;
		int startup;
		if((startup = WSAStartup(wsock_ver, &wsock_data)) != 0){
			LOGGER << "winsock startup error " << startup;
			exit(1);
		}
		#endif

		//instantiate networking subsystem
		Reactor = new reactor_select(port);

		for(int x=0; x<4; ++x){
			Workers.create_thread(boost::bind(&network::worker_pool, this));
		}
	}

	~network()
	{
		Workers.interrupt_all();
		Workers.join_all();

		//deleting reactor stops internal reactor thread
		delete Reactor;

		#ifdef WIN32
		WSACleanup();
		#endif
	}

	/*
	Establish connection to specified IP and port. Returns true if connection
	will be tried, or false if outgoing_connection limit reached.
	*/
	bool add_connection(const std::string & IP, const int port)
	{
		int new_FD = socket(PF_INET, SOCK_STREAM, 0);
		if(new_FD == -1){
			#ifdef WIN32
			LOGGER << "winsock error " << WSAGetLastError();
			#else
			perror("socket");
			#endif
			return false;
		}

		//set socket to non-blocking for async connect
		fcntl(new_FD, F_SETFL, O_NONBLOCK);

		sockaddr_in dest;
		dest.sin_family = AF_INET;   //IPV4
		dest.sin_port = htons(port); //port to connect to

		/*
		The inet_aton function is not used because winsock doesn't have it. Instead
		the inet_addr function is used which is 'almost' equivalent.

		If you try to connect to 255.255.255.255 the return value is -1 which is the
		correct conversion, but also the same return as an error. This is ambiguous
		so a special case is needed for it.
		*/
		if(IP == "255.255.255.255"){
			dest.sin_addr.s_addr = inet_addr(IP.c_str());
		}else if((dest.sin_addr.s_addr = inet_addr(IP.c_str())) == -1){
			LOGGER << "invalid IP " << IP;
			return false;
		}

		/*
		The sockaddr_in struct is smaller than the sockaddr struct. The sin_zero
		member of sockaddr_in is padding to make them the same size. It is required
		that the sine_zero space be zero'd out. Not doing so can result in undefined
		behavior.
		*/
		std::memset(&dest.sin_zero, 0, sizeof(dest.sin_zero));

		if(connect(new_FD, (sockaddr *)&dest, sizeof(dest)) == -1){
			//socket in progress of connecting
			LOGGER << "async connection " << IP << ":" << port << " socket " << new_FD;
//DEBUG, register with reactor
		}else{
			//socket connected right away, rare but it might happen
			LOGGER << "connection " << IP << ":" << port << " socket " << new_FD;
//DEBUG, register with reactor
		}
		return true;
	}

private:
	//reactor for networking
	reactor * Reactor;

	//worker threads to handle call backs
	boost::thread_group Workers;

	//call backs
	boost::function<void (socket_data &)> connect_call_back;
	boost::function<void (socket_data &)> disconnect_call_back;

	void worker_pool()
	{
		boost::shared_ptr<reactor::socket_data> info;
		while(true){
			boost::this_thread::interruption_point();

			Reactor->get_job(info);

			if(!info->connect_flag){
				info->connect_flag = true;
				connect_call_back(info->Socket_Data_Visible);
			}

			if(info->recv_flag){
				info->recv_flag = false;
				info->Socket_Data_Visible.recv_call_back(info->Socket_Data_Visible);
			}

			if(info->send_flag){
				info->send_flag = false;
				info->Socket_Data_Visible.send_call_back(info->Socket_Data_Visible);
			}

			if(info->disconnect_flag){
				disconnect_call_back(info->Socket_Data_Visible);
			}

			Reactor->finish_job(info);
		}
	}
};
#endif
