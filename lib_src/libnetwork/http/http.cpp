#include "http.hpp"

http::http(
	const std::string & web_root_in
):
	web_root(web_root_in),
	State(UNDETERMINED),
	index(0)
{
	std::srand(time(NULL));
}

void http::recv_call_back(network::sock & S)
{
	namespace fs = boost::filesystem;

	if(S.recv_buff.size() > 1024 || State != UNDETERMINED){
		//request size limit exceeded or second request made
		S.disconnect_flag = true;
		return;
	}

	//sub-expression 1 is the request path
	boost::regex expression("GET\\s([^\\s]*)(\\s|\\r|\\n)");
	boost::match_results<std::string::iterator> what;
	std::string req = S.recv_buff.str();
	if(boost::regex_search(req.begin(), req.end(), what, expression)){
		std::string temp = what[1];
		replace_encoded_chars(temp);
		//LOGGER << "req " << temp;
		path = fs::system_complete(fs::path(web_root + temp, fs::native));
		if(path.string().find("..") != std::string::npos){
			//stop directory traversal
			State = INVALID;
		}else{
			if(fs::exists(path)){
				if(fs::is_directory(path)){
					fs::path temp = fs::system_complete(fs::path(path.string() + "/index.html", fs::native));
					if(fs::exists(temp)){
						path = temp;
						State = FILE;
					}else{
						State = DIRECTORY;
					}
				}else{
					State = FILE;
				}
			}else{
				State = INVALID;
			}
		}
		read(S);
	}
}

void http::send_call_back(network::sock & S)
{
	if(S.send_buff.size() < MTU){
		read(S);
	}
	if(S.send_buff.empty()){
		//buffer empty after reading means we're done sending
		S.disconnect_flag = true;
	}
}

std::string http::create_header(const unsigned content_length)
{
	std::string header;
	header += "HTTP/1.1 200 OK\r\n";
	header += "Date: Mon, 01 Jan 1970 01:00:00 GMT\r\n";
	header += "Server: test 123\r\n";
	header += "Last-Modified: Mon, 01 Jan 1970 01:00:00 GMT\r\n";

	/*
	Etag is a hash of the page to determine if the client should use a page in
	their cache. We set it to a random string to make the client always get the
	new version.
	*/
	header += "Etag: \"";
	for(int x=0; x<18; ++x){
		header += (char)((std::rand() % 10) + 48); //random '0' to '9'
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

	/*
	Content type is the mime type of the document. If this is not included the
	client has to figure out what to do with the document.
	*/
	//header += "Content-State: text/html; charset=UTF-8\r\n";

	//header must end with blank line
	header += "\r\n";
	return header;
}

void http::read(network::sock & S)
{
	namespace fs = boost::filesystem;
	assert(State != UNDETERMINED);
	if(State == INVALID){
		S.send_buff.append("404");
		State = DONE;
	}else if(State == DIRECTORY){
		std::stringstream ss;
		ss <<
		"<head></head>"
		"<body bgcolor=\"#111111\" text=\"#D8D8D8\" link=\"#FF8C00\" vlink=\"#FF8C00\">"
		"<table><tr><td>Filename</td><td>Size</td></tr>";
		try{
			for(fs::directory_iterator iter_cur(path), iter_end; iter_cur != iter_end; ++iter_cur){
				std::string relative_path = iter_cur->path().directory_string().substr(web_root.size());
				if(fs::is_directory(iter_cur->path())){
					std::string dir_name =  relative_path.substr(relative_path.find_last_of('/') + 1);
					ss << "<tr><td><a href=\"" << relative_path << "\">" << dir_name
					<< "/</a></td><td>DIR</td></tr>";
				}else{
					ss << "<tr><td><a href=\"" << relative_path << "\">" << iter_cur->path().filename()
					<< "</a></td><td>" << convert::size_SI(fs::file_size(iter_cur->path())) << "</td></tr>";
				}
			}
		}catch(std::exception & ex){
			LOGGER << "exception: " << ex.what();
		}
		ss << "</table></body>";
		std::string header = create_header(ss.str().size());
		std::string buff = header + ss.str();
		S.send_buff.append(buff);
		State = DONE;
	}else if(State == FILE){
		if(index == 0){
			boost::uint64_t size;
			try{
				size = boost::filesystem::file_size(path);
			}catch(std::exception & ex){
				LOGGER << "exception: " << ex.what();
				State = DONE;
				return;
			}
			std::string header = create_header(size);
			S.send_buff.append(header.c_str());
		}
		std::fstream fin(path.string().c_str(), std::ios::in | std::ios::binary);
		if(fin.is_open()){
			fin.seekg(index, std::ios::beg);
			S.send_buff.tail_reserve(MTU);
			fin.read(reinterpret_cast<char *>(S.send_buff.tail_start()), S.send_buff.tail_size());
			S.send_buff.tail_resize(fin.gcount());
			index += fin.gcount();
			if(fin.eof()){
				State == DONE;
			}
		}else{
			State == DONE;
		}
	}
}

void http::replace_encoded_chars(std::string & str)
{
	boost::algorithm::replace_all(str, "%20", " ");
	boost::algorithm::replace_all(str, "%27", "'");
}
