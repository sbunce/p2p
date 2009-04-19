//THREADSAFE, THREAD-SPAWNING
#ifndef H_NETWORKING
#define H_NETWORKING

//boost
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

//custom
#include "rate_limit.hpp"

//include
#include <atomic_int.hpp>
#include <buffer.hpp>
#include <logger.hpp>
#include <speed_calculator.hpp>

//networking
#ifdef WIN32
	#define MSG_NOSIGNAL 0  //disable SIGPIPE on send()
	#define FD_SETSIZE 1024
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
	enum direction{
		OUTGOING,
		INCOMING
	};

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
		boost::function<void (int, buffer &, direction)> connect_call_back_in,
		boost::function<bool (int, buffer &, buffer &)> recv_call_back_in,
		boost::function<bool (int, buffer &, buffer &)> send_call_back_in,
		boost::function<void (int)> disconnect_call_back_in,
		const int connect_time_out_in,
		const int socket_time_out_in,
		const int listen_port = -1
	):
		connect_call_back(connect_call_back_in),
		recv_call_back(recv_call_back_in),
		send_call_back(send_call_back_in),
		disconnect_call_back(disconnect_call_back_in),
		begin_FD(0),
		end_FD(1),
		listener(-1),
		writes_pending(0),
		connection_limit(500),
		outgoing_connections(0),
		incoming_connections(0),
		connect_time_out(connect_time_out_in),
		socket_time_out(socket_time_out_in)
	{
		#ifdef WIN32
		WORD wsock_ver = MAKEWORD(1,1);
		WSADATA wsock_data;
		int startup;
		if((startup = WSAStartup(wsock_ver, &wsock_data)) != 0){
			LOGGER << "winsock startup error " << startup;
			exit(1);
		}
		#endif
		FD_ZERO(&master_FDS);
		FD_ZERO(&read_FDS);
		FD_ZERO(&write_FDS);
		FD_ZERO(&connect_FDS);

		if(listen_port != -1){
			start_listener(listen_port);
		}
		start_selfpipe();

		/*
		Priveledge drop after starting listener. We try this when the user is
		trying to bind to a port less than 1024 because we assume the program is
		being run as root.
		*/
		#ifndef WIN32
		if(listen_port < 1024){
//DEBUG, hardcoded for debian
			if(seteuid(33) < 0){
				fprintf(stderr, "Couldn't set uid.\n");
				exit(1);
			}
		}
		#endif

		select_loop_thread = boost::thread(boost::bind(&network::select_loop, this));
	}

	~network()
	{
		select_loop_thread.interrupt();
		select_loop_thread.join();

		//disconnect all sockets
		for(int x=begin_FD; x < end_FD; ++x){
			if(FD_ISSET(x, &master_FDS)){
				LOGGER << "disconnecting socket " << x;
				#ifdef WIN32
				closesocket(x);
				#else
				close(x);
				#endif
			}
		}
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
		boost::recursive_mutex::scoped_lock lock(connection_queue_mutex);
		if(outgoing_connections < connection_limit){
			++outgoing_connections;
			connection_queue.push(std::make_pair(IP, port));
			write(selfpipe_write, "0", 1);
			LOGGER << "queued connection " << IP << ":" << port;
			return true;
		}else{
			LOGGER << "can't queue " << IP << ":" << port << ", connection limit reached";
			return false;
		}
	}

	//returns how many established connections exist
	unsigned connections()
	{
		return outgoing_connections + incoming_connections;
	}

	//returns current download rate (bytes per second)
	unsigned current_download_rate()
	{
		return Rate_Limit.current_download_rate();
	}

	//returns current upload rate (bytes per second)
	unsigned current_upload_rate()
	{
		return Rate_Limit.current_upload_rate();
	}

	//returns IP address that corresponds to socket_FD
	std::string get_IP(const int socket_FD)
	{
		sockaddr_in addr;
		socklen_t len = sizeof(addr);
		getpeername(socket_FD, (sockaddr*)&addr, &len);
		return inet_ntoa(addr.sin_addr);
	}

	//returns port that corresponds to socket_FD
	int get_port(const int socket_FD)
	{
		sockaddr_in addr;
		socklen_t len = sizeof(addr);
		getpeername(socket_FD, (sockaddr*)&addr, &len);
		return ntohs(addr.sin_port);
	}

	//returns download rate limit (bytes per second)
	unsigned get_max_download_rate()
	{
		return Rate_Limit.get_max_download_rate();
	}

	//returns upload rate limit (bytes per second)
	unsigned get_max_upload_rate()
	{
		return Rate_Limit.get_max_upload_rate();
	}

	//sets maximum download rate
	void set_max_download_rate(const unsigned rate)
	{
		return Rate_Limit.set_max_download_rate(rate);
	}

	//sets maximum upload rate
	void set_max_upload_rate(const unsigned rate)
	{
		return Rate_Limit.set_max_upload_rate(rate);
	}

private:
	//thread for main_loop
	boost::thread select_loop_thread;

	fd_set master_FDS;  //master file descriptor set
	fd_set read_FDS;    //what sockets can read non-blocking
	fd_set write_FDS;   //what sockets can write non-blocking
	fd_set connect_FDS; //sockets in progress of connecting

	int begin_FD; //first socket
	int end_FD;   //one past last socket
	int listener; //socket of listener (-1 means no listener)

	/*
	This is used for the self-pipe trick. A pipe is opened and put in the fd_set
	select() will monitor. We can asynchronously signal select() to return by
	writing a byte to the read FD of the pipe.
	*/
	int selfpipe_read;        //put in master_FDS
	int selfpipe_write;       //written to get select to return
	char selfpipe_discard[8]; //used to discard selfpipe bytes

	//call backs
	boost::function<void (int, buffer &, direction)> connect_call_back;
	boost::function<bool (int, buffer &, buffer &)> recv_call_back;
	boost::function<bool (int, buffer &, buffer &)> send_call_back;
	boost::function<void (int)> disconnect_call_back;

	/*
	When no writes are pending CPU time is saved by not checking sockets for
	writeability. When this is zero there are no writes that need to be done.
	Ex: 5 would mean there are 5 send_buff with data in them.
	*/
	int writes_pending;

	/*
	The number of outgoing and incoming connections. Outgoing includes
	connections that are in progress. The connection limit applies to both
	incoming and outgoing (incoming/outgoing may not exceed connection_limit).
	*/
	const int connection_limit;
	atomic_int<int> outgoing_connections;
	atomic_int<int> incoming_connections;

	/*
	connect_time_out - how many seconds until connection attempt cancelled
	socket_time_out  - how many seconds of no I/O before socket disconnected
	*/
	const int connect_time_out;
	const int socket_time_out;

	//data needed for each socket
	class socket_data
	{
	public:
		/*
		The incoming_or_outgoing parameter is either the outgoing_connections or
		incoming_connections variable. It is decremented when the socket_data is
		destroyed.
		*/
		socket_data(
			int & writes_pending_in,
			atomic_int<int> & incoming_or_outgoing_in
		):
			writes_pending(writes_pending_in),
			incoming_or_outgoing(incoming_or_outgoing_in),
			last_seen(std::time(NULL))
		{}
		~socket_data()
		{
			if(!send_buff.empty()){
				--writes_pending;
			}
			--incoming_or_outgoing;
		}
		int & writes_pending;
		atomic_int<int> & incoming_or_outgoing;
		std::time_t last_seen;
		buffer recv_buff;
		buffer send_buff;
	};

	//connected socket associated with socket_data
	std::map<int, boost::shared_ptr<socket_data> > Socket_Data;

	/*
	Connecting socket associated with socket_data. Elements of this will be
	copied to the Socket_Data container when the socket connects.
	*/
	std::map<int, boost::shared_ptr<socket_data> > Connecting_Socket_Data;

	/*
	The add_connection function stores servers to connect to here. This handoff
	is done because it requires less locking.
	std::pair<IP, port>
	*/
	boost::recursive_mutex connection_queue_mutex;
	std::queue<std::pair<std::string, int> > connection_queue;

	//keeps track of total upload/download rates and to do rate limiting
	rate_limit Rate_Limit;

	//add socket to master_FDS, adjusts begin_FD and end_FD
	void add_socket(const int socket_FD)
	{
		assert(socket_FD > 0);
		FD_SET(socket_FD, &master_FDS);
		//sockets in set
		if(socket_FD < begin_FD){
			begin_FD = socket_FD;
		}else if(socket_FD >= end_FD){
			end_FD = socket_FD + 1;
		}
	}

	//disconnect sockets that have timed out
	void check_timeouts()
	{
		//check timeouts on connecting sockets
		std::map<int, boost::shared_ptr<socket_data> >::iterator
		iter_cur = Connecting_Socket_Data.begin(),
		iter_end = Connecting_Socket_Data.end();
		std::time_t current_time = std::time(NULL);
		while(iter_cur != iter_end){
			if(current_time - iter_cur->second->last_seen > connect_time_out){
				LOGGER << "timed out connection attempt to " << iter_cur->first;
				remove_socket(iter_cur->first);
				Connecting_Socket_Data.erase(iter_cur++);
			}else{
				++iter_cur;
			}
		}

		//check timeouts on connected sockets
		iter_cur = Socket_Data.begin();
		iter_end = Socket_Data.end();
		while(iter_cur != iter_end){
			if(current_time - iter_cur->second->last_seen > socket_time_out){
				disconnect_call_back(iter_cur->first);
				remove_socket(iter_cur->first);
				Socket_Data.erase(iter_cur++);
			}else{
				++iter_cur;
			}
		}
	}

	/*
	This function will do one of the following.
	1. If socket has socket_data in Connecting_Socket_Data it will be moved to
	   Socket_Data.
	2. If socket has no socket_data in Connecting_Socket_Data then new
	   socket_data will be created.
	Note: This function does the connect_call_back.
	*/
	socket_data * create_socket_data(const int socket_FD)
	{
		std::map<int, boost::shared_ptr<socket_data> >::iterator SD;
		SD = Socket_Data.find(socket_FD);
		if(SD == Socket_Data.end()){
			std::pair<std::map<int, boost::shared_ptr<socket_data> >::iterator, bool> new_SD;
			direction Direction;
			std::map<int, boost::shared_ptr<socket_data> >::iterator
				CSD = Connecting_Socket_Data.find(socket_FD);
			if(CSD == Connecting_Socket_Data.end()){
				/*
				Socket is incoming connection because it doesn't have
				Connecting_Socket_Data.
				*/
				new_SD = Socket_Data.insert(std::make_pair(socket_FD, new socket_data(writes_pending, incoming_connections)));
				assert(new_SD.second);
				Direction = INCOMING;
			}else{
				/*
				Socket is outgoing connection because it has Connecting_Socket_Data,
				move socket_data from Connecting_Socket_Data to Socket_Data to indicate
				that the socket has connected.
				*/
				FD_CLR(socket_FD, &connect_FDS);
				new_SD = Socket_Data.insert(*CSD);
				assert(new_SD.second);
				new_SD.first->second->last_seen = std::time(NULL);
				Connecting_Socket_Data.erase(socket_FD);
				Direction = OUTGOING;
			}
			bool send_buff_empty = new_SD.first->second->send_buff.empty();
			connect_call_back(socket_FD, new_SD.first->second->send_buff, Direction);
			if(send_buff_empty && !new_SD.first->second->send_buff.empty()){
				//send_buff went from empty to non-empty
				++writes_pending;
			}
			return &(*new_SD.first->second);
		}else{
			return &(*SD->second);
		}
	}

	//handle incoming connection on listener
	void establish_incoming_connections()
	{
		int new_FD = 0;
		while(new_FD != -1){
			sockaddr_in remoteaddr;
			socklen_t len = sizeof(remoteaddr);
			new_FD = accept(listener, (sockaddr *)&remoteaddr, &len);
			if(new_FD != -1){
				//connection established
				std::string new_IP(inet_ntoa(remoteaddr.sin_addr));
				LOGGER << new_IP << " socket " << new_FD << " connected";
				if(incoming_connections < connection_limit){
					add_socket(new_FD);
					create_socket_data(new_FD);
					++incoming_connections;
				}else{
					LOGGER << "incoming_connection limit exceeded, disconnecting " << new_IP;
					break;
				}
			}
		}
	}

	//called by select_loop to add connections queued by add_connection()
	void establish_outgoing_connection(const std::string & IP, const int port)
	{
		sockaddr_in dest;
		int new_FD = socket(PF_INET, SOCK_STREAM, 0);

		/*
		Set socket to be non-blocking. Trying to read a O_NONBLOCK socket will result
		in -1 being returned and errno would be set to EWOULDBLOCK.
		*/
		fcntl(new_FD, F_SETFL, O_NONBLOCK);

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
			return;
		}

		/*
		The sockaddr_in struct is smaller than the sockaddr struct. The sin_zero
		member of sockaddr_in is padding to make them the same size. It is required
		that the sine_zero space be zero'd out. Not doing so can result in undefined
		behavior.
		*/
		std::memset(&dest.sin_zero, 0, sizeof(dest.sin_zero));

		if(connect(new_FD, (sockaddr *)&dest, sizeof(dest)) == -1){
			/*
			Socket is in progress of connecting. Add to connect_FDS. It will be ready
			to read/write once the socket shows up as writeable.
			*/
			LOGGER << "async connection " << IP << ":" << port << " socket " << new_FD;
			FD_SET(new_FD, &connect_FDS);
		}else{
			//socket connected right away, rare but it might happen
			LOGGER << "connection " << IP << ":" << port << " socket " << new_FD;
		}

		Connecting_Socket_Data.insert(std::make_pair(new_FD, new socket_data(writes_pending, outgoing_connections)));
		add_socket(new_FD);
	}

	//establish queue'd connections
	void process_connection_queue()
	{
		//establish connections queued by add_connection()
		std::pair<std::string, int> new_connect_info;
		while(true){
			{//begin lock scope
			boost::recursive_mutex::scoped_lock lock(connection_queue_mutex);
			if(connection_queue.empty()){
				break;
			}else{
				new_connect_info = connection_queue.front();
				connection_queue.pop();
			}
			}//end lock scope
			establish_outgoing_connection(new_connect_info.first, new_connect_info.second);
		}
	}

	//remove socket from all fd_set's, adjust min_FD and end_FD.
	void remove_socket(const int socket_FD)
	{
		#ifdef WIN32
		closesocket(socket_FD);
		#else
		close(socket_FD);
		#endif
		FD_CLR(socket_FD, &master_FDS);
		FD_CLR(socket_FD, &read_FDS);
		FD_CLR(socket_FD, &write_FDS);
		FD_CLR(socket_FD, &connect_FDS);

		//update end_FD
		for(int x=end_FD; x>begin_FD; --x){
			if(FD_ISSET(x, &master_FDS)){
				end_FD = x + 1;
				break;
			}
		}
		//update begin_FD
		for(int x=begin_FD; x<end_FD; ++x){
			if(FD_ISSET(x, &master_FDS)){
				begin_FD = x;
				break;
			}
		}
	}

	/*
	Accept incoming connections on specified port. Called by ctor if listening
	port is specified as last ctor parameter.
	*/
	void start_listener(const int port)
	{
		//assert not already listening on socket
		assert(listener == -1);

		//set listening socket
		if((listener = socket(PF_INET, SOCK_STREAM, 0)) == -1){
			#ifdef WIN32
			LOGGER << "winsock error " << WSAGetLastError();
			#else
			perror("socket");
			#endif
			exit(1);
		}

		//reuse port if it's currently in use
		int yes = 1;
		if(setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
			#ifdef WIN32
			LOGGER << "winsock error " << WSAGetLastError();
			#else
			perror("setsockopt");
			#endif
			exit(1);
		}

		/*
		Listener set to non-blocking. Also all incoming connections will inherit this.
		*/
		fcntl(listener, F_SETFL, O_NONBLOCK);

		//prepare listener info
		sockaddr_in myaddr;                  //server address
		myaddr.sin_family = AF_INET;         //ipv4
		myaddr.sin_addr.s_addr = INADDR_ANY; //listen on all interfaces
		myaddr.sin_port = htons(port);       //listening port

		//docs for this in establish_connection function
		std::memset(&myaddr.sin_zero, 0, sizeof(myaddr.sin_zero));

		//set listener info
		if(bind(listener, (sockaddr *)&myaddr, sizeof(myaddr)) == -1){
			#ifdef WIN32
			LOGGER << "winsock error " << WSAGetLastError();
			#else
			perror("bind");
			#endif
			exit(1);
		}

		//start listener socket listening
		const int backlog = 64; //how many pending connections can exist
		if(listen(listener, backlog) == -1){
			#ifdef WIN32
			LOGGER << WSAGetLastError();
			#else
			perror("listen");
			#endif
			exit(1);
		}
		LOGGER << "listening socket " << listener << " port " << port;
		add_socket(listener);
	}

	//main network loop
	void select_loop()
	{
		int n_bytes;                 //how many bytes sent/received on send() recv()
		int max_upload;              //max bytes that can be sent to stay under max upload rate
		int max_download;            //max bytes that can be received to stay under max download rate
		int max_upload_per_socket;   //max bytes that can be received to maintain rate limit
		int max_download_per_socket; //max bytes that can be sent to maintain rate limit
		int temp;                    //general purpose
		timeval tv;                  //select() timeout
		bool socket_serviced;        //keeps track of if socket was serviced
		bool send_buff_empty;        //used for pre/post-call_back empty comparison
		int service;                 //set by select to how many sockets need to be serviced
		socket_data * SD;            //holder for create_socket_data return value
		std::time_t last_time(std::time(NULL)); //used to run stuff once per second

		//main loop
		while(true){
			boost::this_thread::interruption_point();

			if(std::time(NULL) != last_time){
				last_time = std::time(NULL);
				check_timeouts();
			}

			process_connection_queue();

			//timeout before select returns with no results
			tv.tv_sec = 0; tv.tv_usec = 10000; // 1/100 second

			//determine what sockets select should monitor
			max_upload = Rate_Limit.upload_rate_control();
			max_download = Rate_Limit.download_rate_control();
			if(writes_pending && max_upload != 0){
				write_FDS = master_FDS;
			}else{
				/*
				When no buffers non-empty we only check for writability on sockets in
				progress of connecting. Writeability on a socket in progress of
				connecting means the socket has connected. This will NOT have the same
				effect as checking for writeability on a connected socket which causes
				100% CPU usage when all buffers are non-empty.
				*/
				write_FDS = connect_FDS;
			}
			if(max_download != 0){
				read_FDS = master_FDS;
			}else{
				FD_ZERO(&read_FDS);
			}

			service = select(end_FD, &read_FDS, &write_FDS, NULL, &tv);

			if(service == -1){
				if(errno != EINTR){
					//fatal error
					#ifdef WIN32
					LOGGER << "winsock error " << WSAGetLastError();
					#else
					perror("select");
					#endif
					exit(1);
				}
			}else if(service != 0){
				/*
				If we were to send the maximum possible amount of data, such that
				we'd stay under the maximum rate, there is a starvation problem that
				can happen. Say we had two sockets and could send two bytes before
				reaching the maximum rate, if we always sent as much as we could,
				and the first socket always took two bytes, the second socket would
				get starved.

				Because of this we set the maximum send such that in the worst case
				each socket (that needs to be serviced) gets to send/recv and equal
				amount of data. It's possible some higher numbered sockets will get
				excluded if there are more sockets than bytes to send()/recv().
				*/
				max_upload_per_socket = max_upload / service;
				max_download_per_socket = max_download / service;
				if(max_upload_per_socket == 0){
					max_upload_per_socket = 1;
				}
				if(max_download_per_socket == 0){
					max_download_per_socket = 1;
				}

				for(int x=begin_FD; x < end_FD && service; ++x){
					socket_serviced = false;

					//bytes from selfpipe are discarded
					if(x == selfpipe_read){
						if(FD_ISSET(x, &read_FDS)){
							//discard recv'd bytes
							read(x, selfpipe_discard, sizeof(selfpipe_discard));
							socket_serviced = true;
						}
						if(FD_ISSET(x, &write_FDS)){
							socket_serviced = true;
						}
						if(socket_serviced){
							--service;
						}
						continue;
					}

					if(FD_ISSET(x, &read_FDS)){
						if(x == listener){
							establish_incoming_connections();
						}else{
							SD = create_socket_data(x);

							/*
							If there are more sockets than bytes to receive there is a case
							where even at 1 byte recv not every socket will get a turn.
							*/
							if(max_download_per_socket > max_download){
								temp = max_download;
							}else{
								temp = max_download_per_socket;
							}

							/*
							The recv size (temp), may be larger than the maximum size the
							buffer tail is allowed to be increased by. If it is then use
							the max increase size.
							*/
							if(temp > 4096){
								SD->recv_buff.tail_reserve(4096);
							}else{
								SD->recv_buff.tail_reserve(temp);
							}

							if(temp != 0){
								/*
								Some implementations might return -1 to indicate an error
								even though select() said the socket was readable. We ignore
								it when that happens.
								*/
								n_bytes = recv(x, SD->recv_buff.tail_start(), SD->recv_buff.tail_size(), MSG_NOSIGNAL);
								if(n_bytes == 0){
									disconnect_call_back(x);
									remove_socket(x);
									Socket_Data.erase(x);
								}else if(n_bytes > 0){
									SD->recv_buff.tail_resize(n_bytes);
									max_download -= n_bytes;
									Rate_Limit.add_download_bytes(n_bytes);
									SD->last_seen = std::time(NULL);
									send_buff_empty = SD->send_buff.empty();
									if(!recv_call_back(x, SD->recv_buff, SD->send_buff)){
										disconnect_call_back(x);
										remove_socket(x);
										Socket_Data.erase(x);
									}else{
										if(send_buff_empty && !SD->send_buff.empty()){
											//send_buff went from empty to non-empty
											++writes_pending;
										}
									}
								}
							}
						}
						socket_serviced = true;
					}

					if(FD_ISSET(x, &write_FDS)){
						SD = create_socket_data(x);
						if(!SD->send_buff.empty()){
							/*
							If there are more sockets than bytes to send there is a case
							where even at 1 byte recv not every socket will get a turn.
							*/
							if(max_upload_per_socket > max_upload){
								temp = max_upload;
							}else{
								temp = max_upload_per_socket;
							}

							//don't try to send more than we have
							if(temp > SD->send_buff.size()){
								temp = SD->send_buff.size();
							}

							if(temp != 0){
								/*
								Some implementations might return -1 to indicate an error
								even though select() said the socket was readable. We ignore
								it when that happens.
								*/
								n_bytes = send(x, SD->send_buff.data(), temp, MSG_NOSIGNAL);
								if(n_bytes == 0){
									disconnect_call_back(x);
									remove_socket(x);
									Socket_Data.erase(x);
								}else if(n_bytes > 0){
									max_upload -= n_bytes;
									Rate_Limit.add_upload_bytes(n_bytes);
									SD->last_seen = std::time(NULL);
									send_buff_empty = SD->send_buff.empty();
									SD->send_buff.erase(0, n_bytes);
									if(!send_buff_empty && SD->send_buff.empty()){
										//send_buff went from non-empty to empty
										--writes_pending;
									}
									send_buff_empty = SD->send_buff.empty();
									if(!send_call_back(x, SD->recv_buff, SD->send_buff)){
										disconnect_call_back(x);
										remove_socket(x);
										Socket_Data.erase(x);
									}else{
										if(send_buff_empty && !SD->send_buff.empty()){
											//send_buff went from empty to non-empty
											++writes_pending;
										}
									}
								}
							}
						}
						socket_serviced = true;
					}

					/*
					Select returns the number of sockets that need to be serviced. By
					keeping track of how many we've seen we might be able to stop
					checking early.
					*/
					if(socket_serviced){
						--service;
					}
				}
			}
		}
	}

	//starts self pipe used to force select to return
	void start_selfpipe()
	{
		int pipe_FD[2];
		if(pipe(pipe_FD) == -1){
			LOGGER << "error creating tickle self-pipe";
			exit(1);
		}
		selfpipe_read = pipe_FD[0];
		selfpipe_write = pipe_FD[1];
		LOGGER << "created self pipe socket " << selfpipe_read;
		fcntl(selfpipe_read, F_SETFL, O_NONBLOCK);
		fcntl(selfpipe_write, F_SETFL, O_NONBLOCK);
		add_socket(selfpipe_read);
	}
};
#endif
