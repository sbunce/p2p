#ifndef H_NETWORK_SYSTEM_INCLUDES
#define H_NETWORK_SYSTEM_INCLUDES

//networking
#ifdef _WIN32
	#define MSG_NOSIGNAL 0   //disable SIGPIPE on send() to disconnected socket
	#define FD_SETSIZE 65536 //max number of sockets in fd_set
	#include <ws2tcpip.h>

	/*
	Macros for BSD errno compatability.
	http://msdn.microsoft.com/en-us/library/ms740668(VS.85).aspx
	Note: Use the BSD style macro then redefine it here.
	*/
	#define socklen_t int              //windows network functions take int, not socklen_t
	#define close closesocket          //close socket function
	#define errno WSAGetLastError()    //error code for last failure
	#define EMFILE WSAEMFILE           //too many open files
	#define EINTR WSAEINTR             //program received interrupt signal (SIGINT)
	#define ENOTCONN WSAENOTCONN       //socket not connected
	#define EINPROGRESS WSAEINPROGRESS //connect in progress
	#define EWOULDBLOCK WSAEWOULDBLOCK //the operation would block
#else
	#include <arpa/inet.h>
	#include <fcntl.h>
	#include <netdb.h>
	#include <netinet/in.h>
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <unistd.h>
#endif

#endif
