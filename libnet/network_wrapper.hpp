#ifndef H_NETWORK_WRAPPER
#define H_NETWORK_WRAPPER

//networking
#ifdef WIN32
	#define MSG_NOSIGNAL 0  //disable SIGPIPE on send() to disconnected socket
	#define FD_SETSIZE 1024 //max number of sockets in fd_set
	#define socklen_t int   //hack for API difference on windows
	#include <winsock.h>
#else
	#include <arpa/inet.h>
	#include <fcntl.h>
	#include <netdb.h>
	#include <netinet/in.h>
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <unistd.h>
#endif

//class to define namespace and keep functions private, it's not instantiable
class network_wrapper
{
public:
	class info
	{
		friend class network_wrapper;
	public:
		info():
			res(NULL),
			ref_count(new int(1))
		{}

		info(const info & Info):
			res(Info.res),
			ref_count(Info.ref_count)
		{
			++(*ref_count);
		}

		~info()
		{
			check_last();
		}

		//returns true if info is good and may be used with other wrapper functions
		bool resolved()
		{
			return res != NULL;
		}

		const info & operator = (const info & rval)
		{
			if(this == &rval){
				//assignment to self, do nothing
			}else{
				//lval might be last reference
				check_last();

				//change what reference this now holds
				res = rval.res;
				ref_count = rval.ref_count;
				++(*ref_count);
			}
		}

	private:
		info(addrinfo * res_in):
			res(res_in),
			ref_count(new int(1))
		{}

		/*
		When res = NULL the info class doesn't contain the address info and will
		trigger an assert if it is used with one of the network functions.
		*/
		addrinfo * res;

		/*
		If ref_count = 1 and dtor called then freeaddrinfo will be called. This is
		the reference counting pointer pattern.
		*/
		int * ref_count;

		void check_last()
		{
			if(*ref_count == 1 && res != NULL){
				freeaddrinfo(res);
			}else{
				--(*ref_count);
			}
		}
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
	Connects to remote host. Returns true if socket is connected (rare if async
	connect with non-blocking socket being done) or false if socket is in
	progress of connecting.
	*/
	static bool connect_socket(const int socket_FD, info & Info)
	{
		assert(Info.res != NULL);
		if(connect(socket_FD, Info.res->ai_addr, Info.res->ai_addrlen) == -1){
			return false;
		}else{
			return true;
		}

		/*
DEBUG, Add support for this:

		This is what would need to happen if trying all returned IPs. This doesn't
		work with async connects. We'd need to store the info until one async
		connect timed out. Then try ai_next etc.

		addrinfo * p;
		for(p = Info.res; p != NULL; p = p->ai_next){
			if((sock_FD = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
				error();
				continue;
			}
			if(connect(socket_FD, p->ai_addr, p->ai_addrlen) == -1){
				disconnect(sockfd);
			}else{
				//connected successfully
				return true;
			}
		}
		return false;
		*/
	}

	/*
	Bind a socket to a specified address and port. Return true if the binding
	succeeded, false if it didn't (port already in use or some other error).

	This only generally needs to be called when setting up a listener. For
	connecting to a remote host a random port is generally used.
	*/
	static bool bind_socket(const int socket_FD, const info & Info)
	{
		if(bind(socket_FD, Info.res->ai_addr, Info.res->ai_addrlen) == -1){
			return false;
		}else{
			return true;
		}
	}

	/*
	Get the OS to allocate a socket. Returns the socket.
	*/
	static int create_socket(const info & Info)
	{
		int socket_FD;
		if((socket_FD = socket(Info.res->ai_family, Info.res->ai_socktype, Info.res->ai_protocol)) == -1){
			LOGGER << "failed to create socket";
			error();
			exit(1);
		}else{
			return socket_FD;
		}
	}

	/*
	Disconnects a socket.
	*/
	static void disconnect(const int socket_FD)
	{
		#ifdef WIN32
		closesocket(socket_FD);
		#else
		close(socket_FD);
		#endif
	}

	/*
	Returns socket information for specified host and port. The host or port may
	be NULL but not both.
	*/
	static info get_info(const char * host, const char * port)
	{
		assert(host != NULL || port != NULL);
		addrinfo hints, *res;
		std::memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;     //IPv4 or IPv6
		hints.ai_socktype = SOCK_STREAM; //TCP
		if(host == NULL){
			//fill in IP (all available interfaces)
			hints.ai_flags = AI_PASSIVE;
		}
		if(getaddrinfo(host, port, &hints, &res) != 0){
			LOGGER << "could not look up host " << host << " port " << port;
			return info(NULL);
		}else{
			return info(res);
		}
	}

	/*
	Many network functions set errno upon error. Winsock also has something
	like errno for displaying errors. This functions prints those.
	*/
	static void error()
	{
		#ifdef WIN32
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
		sockaddr_in6 sa;
		socklen_t len = sizeof(sa);
		getpeername(socket_FD, (sockaddr *)&sa, &len);
		char buff[INET6_ADDRSTRLEN];
		if(inet_ntop(AF_INET6, &sa.sin6_addr, buff, sizeof(buff)) == NULL){
			LOGGER << "could not determine IP";
			return "";
		}else{
			return std::string(buff);
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
		#ifdef WIN32
		const char yes = 1;
		#else
		int yes = 1;
		#endif
		if(setsockopt(socket_FD, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
			error();
		}
	}

	/*
	Sets a socket to be non-blocking.
	*/
	static void set_non_blocking(const int socket_FD)
	{
		//set socket to non-blocking for async connect
		#ifdef WIN32
		u_long mode = 1;
		ioctlsocket(socket_FD, FIONBIO, &mode);
		#else
		fcntl(socket_FD, F_SETFL, O_NONBLOCK);
		#endif
	}

private:
	network_wrapper(){}
};
#endif
