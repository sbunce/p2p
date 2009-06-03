#ifndef H_NETWORK_WRAPPER
#define H_NETWORK_WRAPPER
/*
The network_wrapper namespace contains wrapper functions which simplify using
the BSD sockets networking functions.
*/

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

namespace network_wrapper
{
/*
class addrinfo
{
public:
	addrinfo()
	{
		std::memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;     //IPv4 or IPv6
		hints.ai_socktype = SOCK_STREAM; //TCP
	}

private:
	addrinfo hints, *res;
	getaddrinfo(IP.c_str(), port.c_str(), &hints, &res);
};
*/
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
		//could not determine IP
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

}//end of network_wrapper namespace
#endif
