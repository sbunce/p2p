//THREADSAFE, SINGLETON
#ifndef H_SOCKETS
#define H_SOCKETS

//custom
#include "database.hpp"

//include
#include <atomic_int.hpp>
#include <singleton.hpp>

//networking
#ifdef WIN32
	#define FD_SETSIZE 1024 //max number of connections in FD_SET
	#define MSG_NOSIGNAL 0  //do not signal SIGPIPE on send() error
	#define socklen_t int   //hack for API difference
	#include <winsock.h>
#else
	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <sys/socket.h>
#endif

//std
#include <ctime>

class sockets: public singleton_base<sockets>
{
	friend class singleton_base<sockets>;
public:
	fd_set master_FDS;                //master file descriptor set
	fd_set read_FDS;                  //what sockets can read non-blocking
	fd_set write_FDS;                 //what sockets can write non-blocking
	atomic_int<int> connections;      //currently established connections
	atomic_int<int> max_connections;  //connection limit
	atomic_int<int> FD_max;           //number of the highest socket
	atomic_int<int> localhost_socket; //socket of localhost, or -1 if not connected

	/*
	update_active:
		Updates the time at which the socket last had activity. Do not call this
		function on the listener socket.
	timed_out:
		Returns timed out socket, or -1 if no sockets timed out.
	*/
	void update_active(const int & socket_FD)
	{
		if(FD_ISSET(socket_FD, &master_FDS)){
			std::map<int, time_t>::iterator iter = Active.find(socket_FD);
			if(iter == Active.end()){
				Active.insert(std::make_pair(socket_FD, std::time(NULL)));
			}else{
				iter->second = std::time(NULL);
			}
		}
	}
	int timed_out()
	{
		std::map<int, time_t>::iterator
		iter_cur = Active.begin(),
		iter_end = Active.end();
		while(iter_cur != iter_end){
			if(FD_ISSET(iter_cur->first, &master_FDS)){
				/*
				By checking if settings::FILE_BLOCK_SIZE seconds have passed we
				effectively set the minimum rate to 1B/s.
				*/
				if(std::time(NULL) - iter_cur->second > protocol::FILE_BLOCK_SIZE){
					int tmp = iter_cur->first;
					Active.erase(iter_cur);
					return tmp;
				}else{
					++iter_cur;
				}
			}else{
				Active.erase(iter_cur++);
			}
		}
		return -1;
	}

private:
	sockets():
		connections(0),
		FD_max(0),
		localhost_socket(-1)
	{
		FD_ZERO(&master_FDS);
		FD_ZERO(&read_FDS);
		FD_ZERO(&write_FDS);

		database::connection DB;
		max_connections = database::table::preferences::get_max_connections(DB);
	}

	//socket mapped to time at which the socket last had activity
	std::map<int, time_t> Active;
};
#endif
