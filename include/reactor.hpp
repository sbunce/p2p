/*
Only have connect/disconnect call backs. Make send/recv call backs be registered
during connect call back. Objects to call back to can be stored in container
associated with socket. They can be removed during disconnect call back.
*/

#ifndef H_REACTOR
#define H_REACTOR

class reactor
{
public:
	reactor(){}
	virtual ~reactor(){}

	//data needed for each socket
	class socket_data
	{
	public:
		socket_data(
			const int socket_FD_in,
			const std::string & IP_in,
			const int port_in
		):
			socket_FD(socket_FD_in),
			IP(IP_in),
			port(port_in),
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
		{}

		int socket_FD;
		std::time_t last_seen; //last time socket had activity
		std::string IP;        //stored here so IP of disconnected socket can be known
		int port;              //stored here so port of disconnected socket can be known

		/*
		connect_flag:
			If this is false the connect call back needs to be done. After the call
			back is done this is set to true.
		recv_flag:
			The reactor sets this to true when recv call back needs to be done.
		send_flag:
			The reactor sets this to true when send call back needs to be done.
		disconnect_flag:
			If this is true after get_job returns the reactor disconnected the
			socket (because the other end hung up). If this is true when the
			socket_data is passed to finish_job the socket will be disconnected if
			it wasn't already.			
		*/
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

			/*
			This should not be used to send/recv any data. It's only included to
			serve as a convenient index to a std::map or some other data structure
			to keep track of connection info.
			*/
			const int socket_FD;

			const std::string & IP;
			const int port;
			buffer & recv_buff;
			buffer & send_buff;

			/*
			If this is set to true in the call back the socket will be disconnected
			immediately.
			*/
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
	get_job:
		Blocks until a job is ready. The job type is stored in socket_data
		Socket_State variable. This function is intended to be called by multiple
		worker threads to wait for jobs and do call backs.
	finish_job:
		After worker does call backs this function will be called so that the
		socket is again monitored for activity.
	*/
	virtual void get_job(boost::shared_ptr<socket_data> & info) = 0;
	virtual void finish_job(boost::shared_ptr<socket_data> & info) = 0;

protected:
	//connected socket associated with socket_data
	std::map<int, boost::shared_ptr<socket_data> > Socket_Data;
};
#endif
