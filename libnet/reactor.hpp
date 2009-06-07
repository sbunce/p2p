#ifndef H_REACTOR
#define H_REACTOR

//include
#include <buffer.hpp>
#include <logger.hpp>

//networking
#include "network_wrapper.hpp"
#include "rate_limit.hpp"

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
			const std::string & host_in,
			const std::string & IP_in,
			const std::string & port_in,
			network_wrapper::info Info_in = network_wrapper::info()
		):
			socket_FD(socket_FD_in),
			host(host_in),
			IP(IP_in),
			port(port_in),
			Info(Info_in),
			failed_connect_flag(false),
			connect_flag(false),
			recv_flag(false),
			send_flag(false),
			disconnect_flag(false),
			last_seen(std::time(NULL)),
			Socket_Data_Visible(
				socket_FD,
				host,
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

		std::string host; //name we connected to (ie "google.com")
		std::string IP;   //IP host resolved to
		std::string port; //if listen_port == port then connection is incoming

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
		This is needed if the host resolves to multiple IPs. This is used to try
		the next IP in the list until connection happens or we run out of IPs. 
		However, if the first IP is connected to, no other IP's will be connected
		to.
		*/
		network_wrapper::info Info;

		/*
		This is typedef'd to socket in network::. The purpose of this class is to
		limit what data the call back has access to. For example the call back
		should not have access to any flag but the disconnect flag.
		*/
		class socket_data_visible
		{
		public:
			socket_data_visible(
				const int socket_FD_in,
				const std::string & host_in,
				const std::string & IP_in,
				const std::string & port_in,
				bool & disconnect_flag_in,
				buffer & recv_buff_in,
				buffer & send_buff_in
			):
				socket_FD(socket_FD_in),
				host(host_in),
				IP(IP_in),
				port(port_in),
				disconnect_flag(disconnect_flag_in),
				recv_buff(recv_buff_in),
				send_buff(send_buff_in)
			{}

			//const references to save space, non-const references may be changed
			const int socket_FD;      //WARNING: do not write this
			const std::string & host; //empty when incoming connection
			const std::string & IP;   //IP address host resolved to
			const std::string & port; //port on remote end
			buffer & recv_buff;
			buffer & send_buff;
			bool & disconnect_flag;   //trigger disconnect when set to true

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
	virtual bool connect_to(const std::string & host, const std::string & port) = 0;
	virtual void get_job(boost::shared_ptr<socket_data> & info) = 0;
	virtual void finish_job(boost::shared_ptr<socket_data> & info) = 0;

protected:
	//connected socket associated with socket_data
	std::map<int, boost::shared_ptr<socket_data> > Socket_Data;

	//controls max upload/download
	rate_limit Rate_Limit;
};
#endif
