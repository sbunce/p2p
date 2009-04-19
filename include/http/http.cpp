#include "http.hpp"

http::http(
	const std::string & web_root_in,
	const unsigned max_upload_rate
):
	Network(
		boost::bind(&http::connect_call_back, this, _1, _2, _3),
		boost::bind(&http::recv_call_back, this, _1, _2, _3),
		boost::bind(&http::send_call_back, this, _1, _2, _3),
		boost::bind(&http::disconnect_call_back, this, _1),
		32,
		32,
		8080
	),
	web_root(web_root_in)
{
	//Network.set_max_upload_rate(max_upload_rate);
}

void http::connect_call_back(const int socket_FD, buffer & send_buff, network::direction Direction)
{
	LOGGER << socket_FD << ":" << Network.get_IP(socket_FD) << ":"
		<< Network.get_port(socket_FD);
	Connection.insert(std::make_pair(socket_FD, connection()));
}

bool http::recv_call_back(const int socket_FD, buffer & recv_buff, buffer & send_buff)
{
	namespace fs = boost::filesystem;
	std::map<int, connection>::iterator iter = Connection.find(socket_FD);
	assert(iter != Connection.end());

	if(recv_buff.size() > 1024 || iter->second.Type != connection::UNDETERMINED){
		//request size limit exceeded
		return false;
	}

	int pos_1, pos_2;
	if((pos_1 = recv_buff.find((unsigned char *)"\r\n\r\n", 4)) != buffer::npos){
		if((pos_1 = recv_buff.find((unsigned char *)"GET ", 4)) != buffer::npos){
			if(pos_1 == buffer::npos){
				//GET request not found
				return false;
			}
			pos_1 += 4;
			pos_2 = recv_buff.find((unsigned char *)" ", 1, pos_1);
			if(pos_1 == buffer::npos){
				//end of path not found
				return false;
			}
			std::string get_path;
			for(int x=pos_1; x<pos_2; ++x){
				get_path += recv_buff[x];	
			}
			replace_encoded_chars(get_path);
			iter->second.get_path = get_path;
			iter->second.path = fs::system_complete(fs::path(web_root + get_path, fs::native));
			LOGGER << "GET " << get_path;
			determine_type(iter->second);
			read(iter->second, send_buff);
		}else{
			//invalid request
			return false;
		}
	}
	return true;
}

bool http::send_call_back(const int socket_FD, buffer & recv_buff, buffer & send_buff)
{
	if(send_buff.empty()){
		std::map<int, connection>::iterator iter = Connection.find(socket_FD);
		assert(iter != Connection.end());
		read(iter->second, send_buff);
		return !send_buff.empty();
	}else{
		return true;
	}
}

void http::disconnect_call_back(const int socket_FD)
{
	LOGGER << socket_FD << ":" << Network.get_IP(socket_FD) << ":"
		<< Network.get_port(socket_FD);
	Connection.erase(socket_FD);
}

void http::determine_type(connection & Conn)
{
	namespace fs = boost::filesystem;
	assert(Conn.path.string().size() > 0);
	if(Conn.path.string().find("..") != std::string::npos){
		//stop directory traversal
		Conn.Type = connection::INVALID;
	}else{
		if(fs::exists(Conn.path)){
			if(fs::is_directory(Conn.path)){
				Conn.Type = connection::DIRECTORY;	
				if(!Conn.get_path.empty() && Conn.get_path[Conn.get_path.size() - 1] != '/'){
					Conn.get_path += '/';
				}
				//see if index.html exists in directory
				fs::path temp = fs::system_complete(fs::path(Conn.path.string() + "/index.html", fs::native));
				if(fs::exists(temp)){
					Conn.get_path += "index.html";
					Conn.path = temp;
					Conn.Type = connection::FILE;
				}
			}else{
				Conn.Type = connection::FILE;
			}
		}else{
			Conn.Type = connection::INVALID;
		}
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

void http::read(connection & Conn, buffer & send_buff)
{
	namespace fs = boost::filesystem;
	assert(Conn.Type != connection::UNDETERMINED);

	if(Conn.Type == connection::INVALID){
		send_buff.append((unsigned char *)"404", 3);
		Conn.Type = connection::DONE;
	}else if(Conn.Type == connection::DIRECTORY){
		std::stringstream ss;
		ss << "<head>\n"
			<< "</head>\n"
			<< "<body bgcolor=\"#111111\" text=\"#D8D8D8\" link=\"#FF8C00\" vlink=\"#FF8C00\">\n"
			<< "<table>\n"
			<< "<tr><td>Filename</td><td>Size</td></tr>\n";
		try{
			fs::directory_iterator iter_cur(Conn.path), iter_end;
			while(iter_cur != iter_end){
				if(fs::is_directory(iter_cur->path())){
					std::string dir = iter_cur->path().directory_string();
					dir = dir.substr(web_root.size());
					ss << "<tr>\n"
						<< "<td>\n"
						<< "<a href=\"" << dir << "\">" << dir.substr(dir.find_last_of('/') + 1) << "/" << "</a>\n"
						<< "</td>\n"
						<< "<td>DIR</td>\n";
				}else{
					ss << "<tr>\n"
						<< "<td>\n"
						<< "<a href=\"" << Conn.get_path + iter_cur->path().filename() << "\">" << iter_cur->path().filename() << "</a>\n"
						<< "</td>\n"
						<< "<td>\n"
						<< convert::size_SI(fs::file_size(iter_cur->path()))
						<< "</td>\n";
				}
				++iter_cur;
			}
		}catch(std::exception & ex){
			LOGGER << "exception: " << ex.what();
		}
		ss << "</table>\n"
			<< "</body>";
		send_buff.append((unsigned char *)ss.str().data(), ss.str().size());
		Conn.Type = connection::DONE;
	}else if(Conn.Type == connection::FILE){
		std::fstream fin(Conn.path.string().c_str(), std::ios::in | std::ios::binary);
		if(fin.is_open()){
			fin.seekg(Conn.index, std::ios::beg);
			send_buff.tail_reserve(4096);
			fin.read((char *)send_buff.tail_start(), send_buff.tail_size());
			send_buff.tail_resize(fin.gcount());
			Conn.index += fin.gcount();
			if(fin.eof()){
				Conn.Type == connection::DONE;
			}
		}else{
			Conn.Type == connection::DONE;
		}
	}
}

void http::replace_encoded_chars(std::string & get_path)
{
	while(true){
		int pos = get_path.find("%20");
		if(pos == std::string::npos){
			break;
		}else{
			get_path = get_path.replace(pos, 3, 1, ' ');
		}
	}
}
