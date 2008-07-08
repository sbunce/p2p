#include "client_new_connection.h"

client_new_connection::client_new_connection(
	fd_set & master_FDS_in,
	int & FD_max_in
):
	stop_threads(false),
	threads(0),
	Thread_Pool(8)
{
	master_FDS = &master_FDS_in;
	FD_max = &FD_max_in;
}

client_new_connection::~client_new_connection()
{

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

		//if known being blocked wait 1 second then try again
		sleep(1);
	}
}

void client_new_connection::new_connection(download_connection DC)
{
	/*
	If this DC is for a server that is known to have rejected a previous
	connection attempt to it within the last minute then don't attempt another
	connection.
	*/
	if(check_unresponsive(DC.IP)){
		logger::debug(LOGGER_P1,"stopping connection to known unresponsive server ",DC.IP);
		return;
	}

	//do not connect to servers that are blacklisted
	if(DB_blacklist::is_blacklisted(DC.IP)){
		logger::debug(LOGGER_P1,"stopping connection to blacklisted IP ",DC.IP);
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
	inet_aton(DC.IP.c_str(), &dest_addr.sin_addr);
	memset(&(dest_addr.sin_zero),'\0',8);
	if(connect(DC.socket, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0){
		logger::debug(LOGGER_P1,"connection to ",DC.IP," failed");
		add_unresponsive(DC.IP);
	}else{
		logger::debug(LOGGER_P1,"created socket ",DC.socket," for ",DC.IP);
		if(DC.socket > *FD_max){
			*FD_max = DC.socket;
		}

		if(client_buffer::new_connection(DC)){
			/*
			New connection was successfully registered with the client_buffer which
			indicates the download is running normally. Add the socket to the master
			socket set so the client_buffer instantiation for the socket can start
			sending/receiving.
			*/
			FD_SET(DC.socket, master_FDS);
		}else{
			/*
			The download was removed from client_buffer while connection was being
			made. Disconnect the socket since it's not needed.
			*/
			close(DC.socket);
		}
	}
}

void client_new_connection::process_DC(download_connection DC)
{
	++threads;
	block_concurrent(DC);
	new_connection(DC);
	unblock(DC);
	--threads;
}

void client_new_connection::queue(download_connection DC)
{
	/*
	The resolve function uses a blocking function call. Resolution of IPs should
	not be done here.

	When downloads are transitioned this creates a delay which is too high.

	If hostnames are to be resolved it should be done before the download is
	transitioned and the IP should be stored in the DB.

	However host names should probably not be used at all except for in testing.
	*/
	#ifdef RESOLVE_HOST_NAMES
	if(!resolve(DC)){
		logger::debug(LOGGER_P1,"error resolving ",DC.IP);
		return;
	}
	#endif

	if(!client_buffer::add_connection(DC)){
		//no existing client_buffer for the DC, schedule new connection
		Thread_Pool.queue(boost::bind(&client_new_connection::process_DC, this, DC));
	}
}

bool client_new_connection::resolve(download_connection & DC)
{
	//this mess is a reentrant version of gethostname that is added in glibc
	hostent host_buf, *he;
	char * tmp_host_buf;
	int result_code, herr, host_buf_len;
	host_buf_len = 1024;
	tmp_host_buf = (char *)malloc(host_buf_len);
	while((result_code = gethostbyname_r(DC.IP.c_str(), &host_buf, tmp_host_buf, host_buf_len, &he, &herr)) == ERANGE){
		//function returns if supplied buffer wasn't large enough
		host_buf_len *= 2;
		tmp_host_buf = (char *)realloc(tmp_host_buf, host_buf_len);
	}

	if(he == NULL){
		logger::debug(LOGGER_P1,"error resolving ",DC.IP);
		add_unresponsive(DC.IP);
		free(tmp_host_buf);
		return false;
	}

	DC.IP = inet_ntoa(*(struct in_addr*)he->h_addr);
	free(tmp_host_buf);
	return true;
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
