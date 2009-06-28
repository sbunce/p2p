#ifndef H_NETWORK_WRAPPER
#define H_NETWORK_WRAPPER

//include
#include <logger.hpp>

//networking
#ifdef WINDOWS
	#define MSG_NOSIGNAL 0  //disable SIGPIPE on send() to disconnected socket
	#define FD_SETSIZE 1024 //max number of sockets in fd_set
	#include <ws2tcpip.h>
#else
	#include <arpa/inet.h>
	#include <fcntl.h>
	#include <netdb.h>
	#include <netinet/in.h>
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <unistd.h>
#endif

namespace network{
class wrapper
{
public:
	/*
	The getaddrinfo function allocates it's own memory and expects the user to free
	it. This wraps that so that memory is automatically free'd when the object is
	destoyed.
	*/
	class info : private boost::noncopyable
	{
	public:
		info():
			res(NULL),
			res_cur(NULL)
		{}

		/*
		Using this ctor does DNS resolution. At least the host or the
		port must be specified. If both are NULL then resolved() = false.

		The ai_family parameter is the address family to use for the socket.
		AF_INET - IPv4
		AF_INET6 - IPv6
		AF_UNSPEC - IPv4 or IPv6
		*/
		info(const char * host, const char * port, const int ai_family = AF_UNSPEC)
		{
			if(host != NULL || port != NULL){
				addrinfo hints;
				std::memset(&hints, 0, sizeof(hints));
				hints.ai_family = ai_family;
				hints.ai_socktype = SOCK_STREAM; //TCP
				if(host == NULL){
					//fill in IP (all available interfaces)
					hints.ai_flags = AI_PASSIVE;
				}
				if(getaddrinfo(host, port, &hints, &res) != 0){
					res = NULL;
					res_cur = NULL;
				}else{
					res_cur = res;
				}
			}else{
				res = NULL;
				res_cur = NULL;
			}
		}

		~info()
		{
			if(res != NULL){
				freeaddrinfo(res);
			}
		}

		//returns current addrinfo
		addrinfo * get_res() const
		{
			assert(res_cur != NULL);
			return res_cur;
		}

		//traverse to next node in addrinfo list
		void next_res()
		{
			assert(res_cur != NULL);
			res_cur = res_cur->ai_next;
		}

		//returns true if info is good and may be used with other wrapper functions
		bool resolved() const
		{
			if(res == NULL){
				return false;
			}else{
				return res_cur != NULL;
			}
		}

	private:
		/*
		The addrinfo struct is part of a linked list. The pointer to the next link
		is res->ai_next. The res pointer points to the start of the list. The
		res_cur pointer is the current node we're on.
		Note: If res NULL there is no address info.
		Note: If res_cur NULL we're at the end of the list.

		The addrinfo list can have more than one element if the host is multihomed.
		This can happen when the host has both an IPv4 and IPv6 address.
		*/
		addrinfo * res;
		addrinfo * res_cur;
	};

	/*
	Accepts an incoming connection from the specified listener. If the listener
	is set to non-blocking this will return -1 if there is no socket to accept.
	Otherwise the socket_FD will be returned.

	If the listener is non-blocking then any accepted socket will be non-blocking
	also.
	*/
	static int accept_socket(const int listener_FD)
	{
		sockaddr_storage remoteaddr;
		socklen_t len = sizeof(remoteaddr);
		return accept(listener_FD, (sockaddr *)&remoteaddr, &len);
	}

	/*
	When a asynchronously connecting socket (socket set to non-blocking and given
	to connect()) becomes writeable this needs to be called to see if the
	connection was made. If this returns false it means the connection attempt
	failed.
	*/
	static bool async_connect_succeeded(const int socket_FD)
	{
		#ifdef _WIN32
		const char optval = 1;
		#else
		int optval = 1;
		#endif
		int optlen = sizeof(optval);
		getsockopt(socket_FD, SOL_SOCKET, SO_ERROR, &optval, (socklen_t *)&optlen);
		return optval == 0;
	}

	/*
	Bind a socket to a specified address and port. Return true if the binding
	succeeded, false if it didn't (port already in use or some other error).

	This only generally needs to be called when setting up a listener. For
	connecting to a remote host a random port is generally used.
	*/
	static bool bind_socket(const int socket_FD, const info & Info)
	{
		if(bind(socket_FD, Info.get_res()->ai_addr, Info.get_res()->ai_addrlen) == -1){
			return false;
		}else{
			return true;
		}
	}

	/*
	Connects to remote host. Returns true if socket is connected (rare if async
	connect with non-blocking socket being done) or false if socket is in
	progress of connecting.
	*/
	static bool connect_socket(const int socket_FD, const info & Info)
	{
		if(connect(socket_FD, Info.get_res()->ai_addr, Info.get_res()->ai_addrlen) == -1){
			return false;
		}else{
			return true;
		}
	}

	//Return a socket (have OS allocate socket). Returns -1 on error.
	static int create_socket(const info & Info)
	{
		return socket(Info.get_res()->ai_family, Info.get_res()->ai_socktype,
			Info.get_res()->ai_protocol);
	}

	//closes a socket
	static void disconnect(const int socket_FD)
	{
		#ifdef _WIN32
		closesocket(socket_FD);
		#else
		close(socket_FD);
		#endif
	}

	/*
	Many network functions set errno upon error. Winsock also has something
	like errno for displaying errors. This functions prints those.
	*/
	static void error()
	{
		#ifdef _WIN32
		LOGGER << "winsock error " << WSAGetLastError();
		#else
		perror("setsockopt");
		#endif
		exit(1);
	}

	/*
	Returns IP address of remote host. Returns empty string IP cannot be looked up
	(can happen if socket is disconnected).
	*/
	static std::string get_IP(const int socket_FD)
	{
		/*
		Because IPv6 is used all socket addresses need to be gotten as IPv6. If
		the IP is IPv4 then it will be represented as ::ffff:<IPv4 address>
		*/
		sockaddr_storage addr;
		socklen_t len = sizeof(addr);
		getpeername(socket_FD, (sockaddr *)&addr, &len);
		char buff[INET6_ADDRSTRLEN];
		if(addr.ss_family == AF_INET){
			//IPv4 socket
			getnameinfo((sockaddr *)&addr, sizeof(sockaddr_in), buff, sizeof(buff), NULL, 0, NI_NUMERICHOST);
		}else if(addr.ss_family == AF_INET6){
			//IPv6 socket
			getnameinfo((sockaddr *)&addr, sizeof(sockaddr_in6), buff, sizeof(buff), NULL, 0, NI_NUMERICHOST);
		}

		/*
		A dualstack implementation (IPv4/IPv6 are the same code base) will return
		a IPv4 mapped address that will look like this ::ffff:123.123.123.123. We
		remove the front of it.
		*/
		std::string tmp(buff);
		if(tmp.find("::ffff:", 0) == 0){
			return std::string(tmp.begin()+7, tmp.end());
		}else{
			return buff;
		}
	}

	/*
	Same as above except this one takes info. This is good for getting the IP
	when the socket has not yet connected.
	*/
	static std::string get_IP(const info & Info)
	{
		char buff[INET6_ADDRSTRLEN];
		if(Info.get_res()->ai_family == AF_INET){
			//IPv4 socket
			//sockaddr_in addr = addr = &(((sockaddr_in *)Info.get_res()->ai_addr)->sin_addr);
			getnameinfo((sockaddr *)Info.get_res()->ai_addr, sizeof(sockaddr_in), buff, sizeof(buff), NULL, 0, NI_NUMERICHOST);
		}else if(Info.get_res()->ai_family == AF_INET6){
			//IPv6 socket
			//sockaddr_in6 * addr = (sockaddr_in6*)&(((sockaddr_in6 *)Info.get_res()->ai_addr)->sin6_addr);
			getnameinfo((sockaddr *)Info.get_res()->ai_addr, sizeof(sockaddr_in6), buff, sizeof(buff), NULL, 0, NI_NUMERICHOST);
		}

		/*
		A dualstack implementation (IPv4/IPv6 are the same code base) will return
		a IPv4 mapped address that will look like this ::ffff:123.123.123.123. We
		remove the front of it.
		*/
		std::string tmp(buff);
		if(tmp.find("::ffff:", 0) == 0){
			return std::string(tmp.begin()+7, tmp.end());
		}else{
			return buff;
		}
	}

	/*
	Returns port of remote host connected to. Returns empty string if port cannot
	be looked up (can happen if socket is disconnected).
	*/
	static std::string get_port(const int socket_FD)
	{
		sockaddr_in6 sa;
		socklen_t len = sizeof(sa);
		if(getpeername(socket_FD, (sockaddr *)&sa, &len) == -1){
			LOGGER << "could not determine port";
			return "";
		}
		int port;
		port = ntohs(sa.sin6_port);
		std::stringstream ss;
		ss << port;
		return ss.str();
	}

	/*
	This takes care of "address already in use" errors that can happen when
	trying to bind to a port. The error can happen when a listener is setup,
	and the program crashes. The next time the program starts the OS hasn't
	deallocated the port so it won't let you bind to it.
	*/
	static void reuse_port(const int socket_FD)
	{
		#ifdef _WIN32
		const char optval = 1;
		#else
		int optval = 1;
		#endif
		int optlen = sizeof(optval);
		if(setsockopt(socket_FD, SOL_SOCKET, SO_REUSEADDR, &optval, optlen) == -1){
			error();
		}
	}

	//sets a socket to non-blocking
	static void set_non_blocking(const int socket_FD)
	{
		//set socket to non-blocking for async connect
		#ifdef _WIN32
		u_long mode = 1;
		ioctlsocket(socket_FD, FIONBIO, &mode);
		#else
		fcntl(socket_FD, F_SETFL, O_NONBLOCK);
		#endif
	}

	/*
	Not all operating systems have pipes, or a socket_pair function so this
	function is required for portability.
	*/
	static void socket_pair(int & selfpipe_read, int & selfpipe_write)
	{
//DEBUG, try IPv4 then IPv6 if that doesn't work
		//find an available socket by trying to bind to different ports
		int tmp_listener;
		std::string port;
		for(int x=9090; x<65536; ++x){
			std::stringstream ss;
			ss << x;
			info Info("127.0.0.1", ss.str().c_str(), AF_INET);
			assert(Info.resolved());
			tmp_listener = create_socket(Info);
			if(tmp_listener == -1){
				LOGGER << "error creating socket pair";
			}
			reuse_port(tmp_listener);
			if(bind_socket(tmp_listener, Info)){
				port = ss.str();
				break;
			}else{
				disconnect(tmp_listener);
			}
		}

		if(listen(tmp_listener, 1) == -1){
			error();
		}

		//connect socket for writer end of the pair
		info Info("127.0.0.1", port.c_str(), AF_INET);
		assert(Info.resolved());
		selfpipe_write = create_socket(Info);
		if(selfpipe_write == -1){
			LOGGER << "error creating socket pair";
		}
		if(!connect_socket(selfpipe_write, Info)){
			error();
		}

		//accept socket for reader end of the pair
		if((selfpipe_read = accept_socket(tmp_listener)) == -1){
			error();
		}

		disconnect(tmp_listener);
	}

	/*
	Accept incoming connections via IPv4 on specified port. Returns -1 if there
	is an error.
	*/
	static int start_listener_IPv4(const std::string & port)
	{
		int listener = start_listener(port, AF_INET);
		if(listener == -1){
			LOGGER << "error starting IPv4 listener";
		}else{
			LOGGER << "started IPv4 listener port " << port << " sock " << listener;
		}
		return listener;
	}

	/*
	Accept incoming connections via IPv6 on specified port. Returns -1 if there
	is an error.
	*/
	static int start_listener_IPv6(const std::string & port)
	{
		int listener = start_listener(port, AF_INET6);
		if(listener == -1){
			LOGGER << "error starting IPv6 listener";
		}else{
			LOGGER << "started IPv6 listener port " << port << " sock " << listener;
		}
		return listener;
	}

	/*
	Accept incoming connections via IPv4 or IPv6 on specified port. Returns -1 if there
	is an error.
	*/
	static int start_listener_dual_stack(const std::string & port)
	{
		int listener = start_listener(port, AF_UNSPEC);
		if(listener == -1){
			LOGGER << "error starting dual stack listener";
		}else{
			LOGGER << "started dual stack listener port " << port << " sock " << listener;
		}
		return listener;
	}

	/*
	Starts the listeners depending on what system this is. If a listener can't be
	started the socket_FD will be set to -1. If neither listener can be started
	then the program will be terminated. If a dualstack socket is started then
	both listeners will be equal.
	*/
	static void start_listeners(int & listener_IPv4, int & listener_IPv6, const std::string & port)
	{
		#ifdef _WIN32
		listener_IPv4 = wrapper::start_listener_IPv4(port);
		listener_IPv6 = wrapper::start_listener_IPv6(port);
		#else
		listener_IPv4 = wrapper::start_listener_dual_stack(port);
		listener_IPv6 = listener_IPv4;
		#endif
		if(listener_IPv4 == -1 && listener_IPv6 == -1){
			LOGGER << "failed to start either IPv4 or IPv6 listener";
			exit(1);
		}
	}

	//must be called before any networking functions
	static void start_networking()
	{
		#ifdef _WIN32
		WORD wsock_ver = MAKEWORD(2,2);
		WSADATA wsock_data;
		int startup;
		if((startup = WSAStartup(wsock_ver, &wsock_data)) != 0){
			LOGGER << "winsock startup error " << startup;
			exit(1);
		}
		#endif
	}

	//for every call to start_winsock this function must be called
	static void stop_networking()
	{
		#ifdef _WIN32
		WSACleanup();
		#endif
	}
private:
	wrapper();

	static int start_listener(const std::string & port, const int ai_family)
	{
		wrapper::info Info(NULL, port.c_str(), ai_family);
		assert(Info.resolved());
		int listener = create_socket(Info);
		if(listener == -1){
			return -1;
		}
		reuse_port(listener);
		if(!bind_socket(listener, Info)){
			error();
		}
		int backlog = 64;
		if(listen(listener, backlog) == -1){
			error();
		}
		return listener;
	}
};
}//end of network namespace
#endif
