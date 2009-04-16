#include "http.hpp"

http::http():
	Network(
		boost::bind(&http::connect_call_back, this, _1, _2, _3, _4),
		boost::bind(&http::recv_call_back, this, _1, _2, _3),
		boost::bind(&http::send_call_back, this, _1, _2, _3),
		boost::bind(&http::disconnect_call_back, this, _1),
		4,
		4,
		8080
	),
	web_root("/home/smack/Desktop/www")
{
	Network.set_max_upload_rate(1024*1024*5);
}

void http::connect_call_back(int socket_FD, buffer & recv_buff, buffer & send_buff,
	network::direction Direction)
{
	LOGGER << socket_FD;
}

void http::disconnect_call_back(int socket_FD)
{
	LOGGER << socket_FD;
}

void http::process_get(std::string get_command, buffer & send_buff)
{
	namespace fs = boost::filesystem;
	fs::path path = fs::system_complete(fs::path(web_root + get_command.substr(4), fs::native));
	LOGGER << path.string();

	if(fs::exists(path)){
		if(fs::is_directory(path)){
			
		}else{
			boost::uint64_t size = fs::file_size(path);
			std::fstream fin(path.string().c_str(), std::ios::in | std::ios::binary);
			if(fin.is_open()){
				send_buff.resize(size);
				fin.read((char *)send_buff.data(), size);
			}else{
				send_buff.append((unsigned char *)"500", 3);
			}
		}
	}else{
		send_buff.append((unsigned char *)"404", 3);
	}
}

bool http::recv_call_back(int socket_FD, buffer & recv_buff, buffer & send_buff)
{
	if(recv_buff.size() > 1024){
		//request size limit exceeded
		return false;
	}

	int pos_1, pos_2;
	if((pos_1 = recv_buff.find((unsigned char *)"\r\n\r\n", 4)) != buffer::npos){
		if((pos_1 = recv_buff.find((unsigned char *)"GET ", 4)) != buffer::npos){
			if(pos_1 == buffer::npos){
				//GET request found
				return false;
			}
			pos_2 = recv_buff.find((unsigned char *)" ", 1, pos_1 + 4);
			if(pos_1 == buffer::npos){
				//no space to end path found
				return false;
			}
			std::string get_command;
			for(int x=pos_1; x<pos_2; ++x){
				get_command += recv_buff[x];
			}
			process_get(get_command, send_buff);
		}else{
			//invalid request
			return false;
		}
	}
	return true;
}

bool http::send_call_back(int socket_FD, buffer & recv_buff, buffer & send_buff)
{
	if(send_buff.empty()){
		return false;
	}else{
		return true;
	}
}

void http::monitor()
{
	while(true){
		LOGGER << "upload: " << convert::size_SI(Network.current_upload_rate())
		<< " download: " << convert::size_SI(Network.current_download_rate())
		<< " connections: " << Network.connections();
		sleep(5);
	}
}
