#include "client_new_connection.hpp"

client_new_connection::client_new_connection(
	fd_set & master_FDS_in,
	atomic_int<int> & FD_max_in,
	atomic_int<int> & max_connections_in,
	atomic_int<int> & connections_in
):
	Thread_Pool(8),
	max_connections(&max_connections_in),
	connections(&connections_in)
{
	master_FDS = &master_FDS_in;
	FD_max = &FD_max_in;

	#ifdef WIN32
	WORD wsock_ver = MAKEWORD(1,1);
	WSADATA wsock_data;
	int startup;
	if((startup = WSAStartup(wsock_ver, &wsock_data)) != 0){
		LOGGER << "winsock startup error " << startup;
		exit(1);
	}
	#endif
}

client_new_connection::~client_new_connection()
{
	#ifdef WIN32
	WSACleanup();
	#endif
}

void client_new_connection::add_unresponsive(const std::string & IP)
{
	boost::mutex::scoped_lock lock(KU_mutex);
	Known_Unresponsive.insert(std::make_pair(time(NULL), IP));
}

bool client_new_connection::check_unresponsive(const std::string & IP)
{
	boost::mutex::scoped_lock lock(KU_mutex);

	//remove elements that are too old
	Known_Unresponsive.erase(Known_Unresponsive.begin(),
		Known_Unresponsive.upper_bound(time(NULL) - global::UNRESPONSIVE_TIMEOUT));

	//see if IP is in the list
	std::map<time_t,std::string>::iterator iter_cur, iter_end;
	iter_cur = Known_Unresponsive.begin();
	iter_end = Known_Unresponsive.end();
	while(iter_cur != iter_end){
		if(iter_cur->second == IP){
			return true;
		}else{
			++iter_cur;
		}
	}
	return false;
}

void client_new_connection::block_concurrent(const download_connection & DC)
{
	/*
	If two more DC's with the same IP reach here only one will be let past and
	the others will be blocked until the one that got past is sent to DC_unblock().
	*/
	while(true){
		{
		boost::mutex::scoped_lock lock(CCA_mutex);
		bool found = false;
		std::list<download_connection>::iterator iter_cur, iter_end;
		iter_cur = Connection_Current_Attempt.begin();
		iter_end = Connection_Current_Attempt.end();
		while(iter_cur != iter_end){
			if(iter_cur->IP == DC.IP){
				found = true;
				break;
			}
			++iter_cur;
		}
		if(!found){
			Connection_Current_Attempt.push_back(DC);
			break;
		}
		}

		//if known DC being blocked wait 1/10 second then try again
		portable_sleep::ms(100);
	}
}

void client_new_connection::new_connection(download_connection DC)
{
	/*
	Do not initiate new connections if at connection limit.
	*/
	if(*connections >= *max_connections){
		return;
	}

	/*
	If this DC is for a server that is known to have rejected a previous
	connection attempt to it within the last minute then don't attempt another
	connection.
	*/
	if(check_unresponsive(DC.IP)){
		LOGGER << "stopping connection to known unresponsive server " << DC.IP;
		return;
	}

	//do not connect to servers that are blacklisted
	if(DB_Blacklist.is_blacklisted(DC.IP)){
		LOGGER << "stopping connection to blacklisted IP " << DC.IP;
		return;
	}

	/*
	It's possible the download was stopped while the DC was in the thread pool.
	If it was stopped abort connection attempt.
	*/
	if(!client_buffer::is_downloading(DC.Download)){
		return;
	}

	sockaddr_in dest_addr;
	DC.socket = socket(PF_INET, SOCK_STREAM, 0);
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(global::P2P_PORT);

	#ifdef WIN32
	dest_addr.sin_addr.s_addr = inet_addr(DC.IP.c_str());
	#else
	inet_aton(DC.IP.c_str(), &dest_addr.sin_addr);
	#endif

	memset(&(dest_addr.sin_zero),'\0',8);

	/*
	Before making new connection check to see if a connection was already
	created to this server.
	*/
	if(client_buffer::add_connection(DC)){
		return;
	}

	if(connect(DC.socket, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0){
		LOGGER << "connection to " << DC.IP << " failed";
		add_unresponsive(DC.IP);
	}else{
		LOGGER << "created socket " << DC.socket << " for " << DC.IP;
		if(DC.socket > *FD_max){
			*FD_max = DC.socket;
		}
		client_buffer::new_connection(DC);
		FD_SET(DC.socket, master_FDS);
		++*connections;
	}
}

void client_new_connection::process_DC(download_connection DC)
{
	block_concurrent(DC);
	new_connection(DC);
	unblock(DC);
}

void client_new_connection::queue(download_connection DC)
{
	//resolve hostname
	if(!resolve::hostname(DC.IP)){
		LOGGER << "failed to resolve " << DC.IP;
		return;
	}

	#ifndef ALLOW_LOCALHOST_CONNECTION
	//stop connections to localhost
	if(DC.IP.find("127.") != std::string::npos){
		LOGGER << "stopping connection to localhost";
		return;
	}
	#endif

	/*
	Before using a thread, check to see if a connection to this server already
	exists. If it does add the download connection to it.
	*/
	if(client_buffer::add_connection(DC)){
		return;
	}

	Thread_Pool.queue(boost::bind(&client_new_connection::process_DC, this, DC));
}

void client_new_connection::unblock(const download_connection & DC)
{
	/*
	Remove this server from the current connection attempts to let DC_block_concurrent
	let another download_connection to this server through.
	*/
	boost::mutex::scoped_lock lock(CCA_mutex);
	std::list<download_connection>::iterator iter_cur, iter_end;
	iter_cur = Connection_Current_Attempt.begin();
	iter_end = Connection_Current_Attempt.end();
	while(iter_cur != iter_end){
		if(iter_cur->IP == DC.IP && iter_cur->Download == DC.Download){
			Connection_Current_Attempt.erase(iter_cur);
			break;
		}
		++iter_cur;
	}
}
