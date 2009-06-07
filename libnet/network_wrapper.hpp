#ifndef H_NETWORK_WRAPPER
#define H_NETWORK_WRAPPER

//networking
#ifdef WIN32
	#define MSG_NOSIGNAL 0  //disable SIGPIPE on send() to disconnected socket
	#define FD_SETSIZE 1024 //max number of sockets in fd_set
	#define socklen_t int   //hack for API difference on windows
	#include <winsock2.h>
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

//class to define namespace and keep functions private, it's not instantiable
class network_wrapper
{
public:

	class info
	{
	public:
		info(){}

		/*
		This ctor called by get_info function. The addrinfo pointer passed to this
		ctor should not have freeaddrinfo() called on it, and the pointer should
		not be passed to the ctor of any other info object. The addrinfo will
		automatically be free'd when last reference to this class destroyed.
		*/
		info(addrinfo * res_in)
		{
			Ref = boost::shared_ptr<ref>(new ref(res_in));
		}

		/*
		Returns stored addrinfo. This function should not be called if 
		resolved() == false.
		*/
		addrinfo * get_res() const
		{
			assert(Ref.get() != NULL);
			return Ref->res_cur;
		}

		/*
		Makes the info container point to the next addrinfo. After calling this
		resolved() should be called to make sure we didn't hit the end of the
		list.
		*/
		void next_res()
		{
			Ref->res_cur = Ref->res_cur->ai_next;
		}

		//returns true if info is good and may be used with other wrapper functions
		bool resolved() const
		{
			if(Ref.get() == NULL){
				return false;
			}else{
				return Ref->res_cur != NULL;
			}
		}

	private:
		//facilitates doing reference count with boost::shared_ptr
		class ref
		{
		public:
			ref(addrinfo * res_in):
				res(res_in),
				res_cur(res_in)
			{}

			~ref()
			{
				freeaddrinfo(res);
			}

			/*
			Pointer to the first addrinfo in the list.
			*/
			addrinfo * res;

			/*
			Pointer to current element in list. Initially this points to res. If
			this is null there are no more results.
			*/
			addrinfo * res_cur;
		};

		boost::shared_ptr<ref> Ref;
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
		assert(Info.resolved());
		if(connect(socket_FD, Info.get_res()->ai_addr, Info.get_res()->ai_addrlen) == -1){
			return false;
		}else{
			return true;
		}
	}

	/*
	Bind a socket to a specified address and port. Return true if the binding
	succeeded, false if it didn't (port already in use or some other error).

	This only generally needs to be called when setting up a listener. For
	connecting to a remote host a random port is generally used.
	*/
	static bool bind_socket(const int socket_FD, const info & Info)
	{
		assert(Info.resolved());
		if(bind(socket_FD, Info.get_res()->ai_addr, Info.get_res()->ai_addrlen) == -1){
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
		assert(Info.resolved());
		int socket_FD;
		if((socket_FD = socket(Info.get_res()->ai_family, Info.get_res()->ai_socktype,
			Info.get_res()->ai_protocol)) == -1)
		{
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
		/*
		Because IPv6 is used all socket addresses need to be gotten as IPv6. If
		the IP is IPv4 then it will be represented as ::ffff:<IPv4 address>
		*/
		sockaddr_in6 sa;
		socklen_t len = sizeof(sa);
		getpeername(socket_FD, (sockaddr *)&sa, &len);
		char buff[INET6_ADDRSTRLEN];
		if(inet_ntop(AF_INET6, &sa.sin6_addr, buff, sizeof(buff)) == NULL){
			LOGGER << "could not determine IP";
			return "";
		}else{
			std::string tmp(buff);
			size_t loc;
			if((loc = tmp.find("::ffff:")) == 0){
				//IPv4, trimm off "::ffff:"
				return std::string(tmp.begin()+7, tmp.end());
			}else{
				//IPv6
				return tmp;
			}
		}
	}

	/*
	Same as above except this one takes info. This is good for getting the IP
	when the socket has not yet connected.
	*/
	static std::string get_IP(const info & Info)
	{
		void * addr;
		if(Info.get_res()->ai_family == AF_INET){
			addr = &(((sockaddr_in *)Info.get_res()->ai_addr)->sin_addr);
		}else if(Info.get_res()->ai_family == AF_INET6){
			addr = &(((sockaddr_in6 *)Info.get_res()->ai_addr)->sin6_addr);
		}else{
			LOGGER << "unknown address family";
		}
		char buff[INET6_ADDRSTRLEN];
		if(inet_ntop(Info.get_res()->ai_family, addr, buff, sizeof(buff)) == NULL){
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
