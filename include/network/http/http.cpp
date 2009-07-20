#include "http.hpp"

http::http(
	const std::string & web_root_in
):
	web_root(web_root_in),
	Type(UNDETERMINED),
	index(0)
{
	std::srand(time(NULL));
}

void http::recv_call_back(network::socket_data & SD)
{
	namespace fs = boost::filesystem;

	if(SD.recv_buff.size() > 1024 || Type != UNDETERMINED){
		//request size limit exceeded or second request made
		SD.disconnect_flag = true;
		return;
	}

	int pos_1, pos_2;
	if(SD.recv_buff.size() >= 6 &&
		(pos_1 = SD.recv_buff.find((unsigned char *)"GET ", 4)) != network::buffer::npos &&
		(pos_2 = SD.recv_buff.find((unsigned char *)"\n", 1, pos_1)) != network::buffer::npos
	){
		//path terminated by ' ', '\r', or '\n'
		for(int x=pos_1+4; x<pos_2; ++x){
			if(SD.recv_buff[x] == ' ' ||
				SD.recv_buff[x] == '\r' ||
				SD.recv_buff[x] == '\n'
			){
				break;
			}else{
				get_path += SD.recv_buff[x];
			}
		}

		replace_encoded_chars();
		path = fs::system_complete(fs::path(web_root + get_path, fs::native));
		//LOGGER << "req " << path.string();
		determine_type();
		read(SD.send_buff);
	}
}

void http::send_call_back(network::socket_data & SD)
{
	read(SD.send_buff);
	if(SD.send_buff.empty()){
		SD.disconnect_flag = true;
	}
}

void http::determine_type()
{
	namespace fs = boost::filesystem;
	assert(path.string().size() > 0);
	if(path.string().find("..") != std::string::npos){
		//stop directory traversal
		Type = INVALID;
	}else{
		if(fs::exists(path)){
			if(fs::is_directory(path)){
				Type = DIRECTORY;	
				if(!get_path.empty() && get_path[get_path.size() - 1] != '/'){
					get_path += '/';
				}
				//see if index.html exists in directory
				fs::path temp = fs::system_complete(fs::path(path.string() + "/index.html", fs::native));
				if(fs::exists(temp)){
					get_path += "index.html";
					path = temp;
					Type = FILE;
				}
			}else{
				Type = FILE;
			}
		}else{
			Type = INVALID;
		}
	}
}

std::string http::create_header(const unsigned content_length)
{
	/* Example Header:
	HTTP/1.1 200 OK
	Date: Mon, 01 Jan 1970 01:00:00 GMT
	Server: test
	Last-Modified: Mon, 01 Jan 1970 01:00:00 GMT
	Etag: "3f80f-1b6-3e1cb03b"
	Accept-Ranges: bytes
	Content-Length: 438
	Connection: close
	Content-Type: text/html; charset=UTF-8
	*/

	std::string header;
	header += "HTTP/1.1 200 OK\r\n";
	header += "Date: Mon, 01 Jan 1970 01:00:00 GMT\r\n";
	header += "Server: test 123\r\n";
	header += "Last-Modified: Mon, 01 Jan 1970 01:00:00 GMT\r\n";

	//some sort of page hash (determines if browser uses cached version)
	header += "Etag: \"";
	for(int x=0; x<18; ++x){
		//0-9
		header += (char)((std::rand() % 10) + 48);
	}
	header += "\"\r\n";

	//"bytes" means we support partial file requests, any other value means we don't
	header += "Accept-Ranges: unsupported\r\n";

	//length of document following header
	std::stringstream ss;
	ss << content_length;
	header += "Content-Length: " + ss.str() + "\r\n";

	//indicate we will close connection after send done
	header += "Connection: close\r\n";

	//do NOT specify content type, let the client try to figure it out
	//header += "Content-Type: text/html; charset=UTF-8\r\n";

	//header must end with blank line
	header += "\r\n";
	return header;
}

void http::read(network::buffer & send_buff)
{
	namespace fs = boost::filesystem;
	assert(Type != UNDETERMINED);
	if(Type == INVALID){
		send_buff.append((unsigned char *)"404", 3);
		Type = DONE;
	}else if(Type == DIRECTORY){
		std::stringstream ss;
		ss << "<head>\n"
			<< "</head>\n"
			<< "<body bgcolor=\"#111111\" text=\"#D8D8D8\" link=\"#FF8C00\" vlink=\"#FF8C00\">\n"
			<< "<table>\n"
			<< "<tr><td>Filename</td><td>Size</td></tr>\n";
		try{
			fs::directory_iterator iter_cur(path), iter_end;
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
						<< "<a href=\"" << get_path + iter_cur->path().filename() << "\">" << iter_cur->path().filename() << "</a>\n"
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
		std::string header = create_header(ss.str().size());
		std::string buff = header + ss.str();
		send_buff.append((unsigned char *)buff.data(), buff.size());
		Type = DONE;
	}else if(Type == FILE){
		boost::uint64_t size;
		try{
			size = boost::filesystem::file_size(path);
		}catch(std::exception & ex){
			LOGGER << "exception: " << ex.what();
			exit(1);
		}
		std::fstream fin(path.string().c_str(), std::ios::in | std::ios::binary);
		if(fin.is_open()){
			fin.seekg(index, std::ios::beg);
			char buff[4096];
			fin.read(buff, sizeof(buff));
			std::string header = create_header(size);
			send_buff.append((unsigned char *)header.data(), header.size());
			send_buff.append((unsigned char *)buff, fin.gcount());
			index += fin.gcount();
			if(fin.eof()){
				Type == DONE;
			}
		}else{
			Type == DONE;
		}
	}
}

void http::replace_encoded_chars()
{
	while(true){
		int pos = get_path.find("%20");
		if(pos == std::string::npos){
			break;
		}else{
			get_path = get_path.replace(pos, 3, 1, ' ');
		}
	}

	while(true){
		int pos = get_path.find("%27");
		if(pos == std::string::npos){
			break;
		}else{
			get_path = get_path.replace(pos, 3, 1, '\'');
		}
	}
}
