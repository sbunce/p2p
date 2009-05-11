#ifndef H_REACTOR
#define H_REACTOR

//include
#include <buffer.hpp>
#include <logger.hpp>

//networking
#ifdef WIN32
	#define MSG_NOSIGNAL 0  //disable SIGPIPE on send()
	#define FD_SETSIZE 1024 //max number of sockets in fd_set
	#define socklen_t int   //hack for little API difference on windows
	#include <winsock.h>
#else
	#include <arpa/inet.h>
	#include <fcntl.h>
	#include <netinet/in.h>
	#include <sys/socket.h>
	#include <unistd.h>
#endif

class reactor
{
public:
	reactor()
	{
		#ifdef WIN32
//DEBUG, move this up to 2,2?
		WORD wsock_ver = MAKEWORD(1,1);
		WSADATA wsock_data;
		int startup;
		if((startup = WSAStartup(wsock_ver, &wsock_data)) != 0){
			LOGGER << "winsock startup error " << startup;
			exit(1);
		}
		#endif
	}

	virtual ~reactor()
	{
		#ifdef WIN32
		WSACleanup();
		#endif
	}

	//data needed for each socket
	class socket_data
	{
	public:
		socket_data(
			const int socket_FD_in,
			const std::string & IP_in,
			const int port_in,
			const bool outgoing_in
		):
			socket_FD(socket_FD_in),
			IP(IP_in),
			port(port_in),
			outgoing(outgoing_in),
			failed_connect_flag(false),
			connect_flag(false),
			recv_flag(false),
			send_flag(false),
			disconnect_flag(false),
			last_seen(std::time(NULL)),
			Socket_Data_Visible(
				socket_FD,
				IP,
				port,
				outgoing,
				disconnect_flag,
				recv_buff,
				send_buff
			)
		{

		}

		int socket_FD;
		std::time_t last_seen; //last time socket had activity
		std::string IP;        //stored here so IP of disconnected socket can be known
		int port;              //stored here so port of disconnected socket can be known
		bool outgoing;         //true if we initiated connected

		/*
		failed_connect_flag:
			If true the failed connect call back needs to be done. No other call
			backs should be done after the failed connect call back.
		connect_flag:
			If false the connect call back needs to be done. After the call back is
			done this is set to true.
		recv_flag:
			If true the recv call back needs to be done.
		send_flag:
			If true the send call back needs to be done.
		disconnect_flag:
			If this is true after get_job returns the reactor disconnected the
			socket (because the other end hung up). If this is true when the
			socket_data is passed to finish_job the socket will be disconnected if
			it wasn't already.			
		*/
		bool failed_connect_flag;
		bool connect_flag;
		bool recv_flag;
		bool send_flag;
		bool disconnect_flag;

		//send()/recv() buffers
		buffer recv_buff;
		buffer send_buff;

		/*
		This is typedef'd to socket_data in network::. The purpose of this class
		is to limit what data the call back gets. It should not have access to the
		flags for example.
		*/
		class socket_data_visible
		{
		public:
			socket_data_visible(
				const int socket_FD_in,
				const std::string & IP_in,
				const int port_in,
				const bool outgoing_in,
				bool & disconnect_flag_in,
				buffer & recv_buff_in,
				buffer & send_buff_in
			):
				socket_FD(socket_FD_in),
				IP(IP_in),
				port(port_in),
				outgoing(outgoing_in),
				disconnect_flag(disconnect_flag_in),
				recv_buff(recv_buff_in),
				send_buff(send_buff_in)
			{}

			const int socket_FD;
			const std::string & IP;
			const int port;
			buffer & recv_buff;
			buffer & send_buff;
			bool outgoing;
			bool & disconnect_flag;

			/*
			These must be set in the connect call back or the program will be
			terminated.
			*/
			boost::function<void (socket_data_visible &)> recv_call_back;
			boost::function<void (socket_data_visible &)> send_call_back;
		};

		socket_data_visible Socket_Data_Visible;
	};

	/*
	add_connection:
		Establishes new connection. Returns false if connection limit reached.
	get_job:
		Blocks until a job is ready. The job type is stored in socket_data
		Socket_State variable. This function is intended to be called by multiple
		worker threads to wait for jobs and do call backs.
	finish_job:
		After worker does call backs this function will be called so that the
		socket is again monitored for activity.
	*/
	virtual bool add_connection(const std::string & IP, const int port) = 0;
	virtual void get_job(boost::shared_ptr<socket_data> & info) = 0;
	virtual void finish_job(boost::shared_ptr<socket_data> & info) = 0;

protected:
	//connected socket associated with socket_data
	std::map<int, boost::shared_ptr<socket_data> > Socket_Data;
};
#endif
