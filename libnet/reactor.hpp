#ifndef H_REACTOR
#define H_REACTOR

//include
#include <buffer.hpp>
#include <logger.hpp>

//networking
#include "network_wrapper.hpp"

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

//DEBUG, could this be moved to network? then make reactor a friend?
	//data needed for each socket
	class socket_data
	{
	public:
		socket_data(
			const int socket_FD_in,
			const std::string & IP_in,
			const std::string & port_in
		):
			socket_FD(socket_FD_in),
			IP(IP_in),
			port(port_in),
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
				disconnect_flag,
				recv_buff,
				send_buff
			)
		{

		}

		int socket_FD;
		std::time_t last_seen; //last time socket had activity

		/*
		IP of remote end. If port is equal to the port that is being listened on
		then this is a connection someone else established with us.
		*/
		std::string IP;
		std::string port;

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
				const std::string & port_in,
				bool & disconnect_flag_in,
				buffer & recv_buff_in,
				buffer & send_buff_in
			):
				socket_FD(socket_FD_in),
				IP(IP_in),
				port(port_in),
				disconnect_flag(disconnect_flag_in),
				recv_buff(recv_buff_in),
				send_buff(send_buff_in)
			{}

			const int socket_FD;
			const std::string & IP;   //reference to save space
			const std::string & port; //reference to save space
			buffer & recv_buff;
			buffer & send_buff;
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
	connect_to:
		Establishes new connection. Returns false if connection limit reached.
	get_job:
		Blocks until a job is ready. The job type is stored in socket_data
		Socket_State variable. This function is intended to be called by multiple
		worker threads to wait for jobs and do call backs.
	finish_job:
		After worker does call backs this function will be called so that the
		socket is again monitored for activity.
	*/
	virtual bool connect_to(const std::string & IP, const std::string & port) = 0;
	virtual void get_job(boost::shared_ptr<socket_data> & info) = 0;
	virtual void finish_job(boost::shared_ptr<socket_data> & info) = 0;

protected:
	//connected socket associated with socket_data
	std::map<int, boost::shared_ptr<socket_data> > Socket_Data;
};
#endif
