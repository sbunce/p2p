#include "http.hpp"

http::http(
	network::proactor & Proactor_in,
	const std::string & web_root_in
):
	Proactor(Proactor_in),
	web_root(web_root_in),
	index(0)
{
	std::srand(time(NULL));
}

void http::recv_call_back(network::connection_info & CI, network::buffer & recv_buf)
{
	namespace fs = boost::filesystem;
	request.append(recv_buf);

	//check if exceeded max request size
	if(request.size() > 1024){
		Proactor.disconnect(CI.connection_ID);
		return;
	}

	//sub-expression 1 is the request path
	boost::regex expression("GET\\s([^\\s]*)(\\s|\\r|\\n)");
	boost::match_results<std::string::iterator> what;
	std::string req = request.str();
	if(boost::regex_search(req.begin(), req.end(), what, expression)){
		std::string temp = what[1];
		decode_chars(temp);
		//LOGGER << "req " << temp;
		path = fs::system_complete(fs::path(web_root + temp, fs::native));
		if(path.string().find("..") != std::string::npos){
			//stop directory traversal
			CI.recv_call_back.clear();
			Proactor.disconnect(CI.connection_ID);
		}else{
			if(fs::exists(path)){
				if(fs::is_directory(path)){
					//check if there is a index file in the directory
					fs::path temp = fs::system_complete(fs::path(path.string() + "/index.html", fs::native));
					if(fs::exists(temp)){
						//serve the index file
						path = temp;
						CI.recv_call_back.clear();
						CI.send_call_back = boost::bind(&http::file_send_call_back, this, _1, _2);
						CI.send_call_back(CI, 0);
					}else{
						//serve directory listing
						CI.recv_call_back.clear();
						read_directory(CI);
					}
				}else{
					//serve file
					CI.recv_call_back.clear();
					CI.send_call_back = boost::bind(&http::file_send_call_back, this, _1, _2);
					CI.send_call_back(CI, 0);
				}
			}else{
				//file at request path doesn't exist
				network::buffer B("404");
				Proactor.write(CI.connection_ID, B);
				Proactor.disconnect_on_empty(CI.connection_ID);
			}
		}
	}
}

std::string http::create_header(const boost::uint64_t & content_length)
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

void http::decode_chars(std::string & str)
{
	boost::algorithm::replace_all(str, "%5B", "[");
	boost::algorithm::replace_all(str, "%5D", "]");
	boost::algorithm::replace_all(str, "%20", " ");
	boost::algorithm::replace_all(str, "%27", "'");
}

void http::encode_chars(std::string & str)
{
	boost::algorithm::replace_all(str, "[", "%5B");
	boost::algorithm::replace_all(str, "]", "%5D");
	boost::algorithm::replace_all(str, " ", "%20");
	boost::algorithm::replace_all(str, "'", "%27");
}

void http::file_send_call_back(network::connection_info & CI, int send_buf_size)
{
	network::buffer B;
	if(index == 0){
		//prepare header
		boost::uint64_t size;
LOGGER << path;
		try{
			size = boost::filesystem::file_size(path);
		}catch(std::exception & ex){
			LOGGER << "exception: " << ex.what();
			CI.send_call_back.clear();
			Proactor.disconnect(CI.connection_ID);
			return;
		}
LOGGER << size;
		std::string header = create_header(size);
LOGGER << header;
		B.append(header);
	}
	if(index == 0 || send_buf_size < MTU){
		std::fstream fin(path.string().c_str(), std::ios::in | std::ios::binary);
		if(fin.is_open()){
			fin.seekg(index, std::ios::beg);
			B.tail_reserve(MTU);
			fin.read(reinterpret_cast<char *>(B.tail_start()), B.tail_size());
			B.tail_resize(fin.gcount());
			index += fin.gcount();
			Proactor.write(CI.connection_ID, B);
			if(fin.eof()){
				CI.send_call_back.clear();
				Proactor.disconnect_on_empty(CI.connection_ID);
			}
		}else{
			CI.send_call_back.clear();
			Proactor.disconnect(CI.connection_ID);
		}
	}
}

void http::read_directory(network::connection_info & CI)
{
	namespace fs = boost::filesystem;
	std::stringstream ss;
	ss <<
	"<head>\n"
	"</head>\n"
	"<body bgcolor=\"#111111\" text=\"#D8D8D8\" link=\"#FF8C00\" vlink=\"#FF8C00\">\n"
	"<table>\n"
	"<tr>\n"
	"<td>name</td>\n"
	"<td>size</td>\n"
	"</tr>\n";
	try{
		for(fs::directory_iterator iter_cur(path), iter_end; iter_cur != iter_end; ++iter_cur){
			std::string relative_path = iter_cur->path().directory_string().substr(web_root.size());
			encode_chars(relative_path);
			std::string file_name = iter_cur->path().filename();
			if(fs::is_directory(iter_cur->path())){
				ss << "<tr>\n<td>\n<a href=\"" << relative_path << "\">" << file_name
					<< "/</a>\n</td>\n<td>DIR</td>\n</tr>";
			}else{
				ss << "<tr>\n<td>\n<a href=\"" << relative_path << "\">" << iter_cur->path().filename()
					<< "</a>\n</td>\n<td>" << convert::size_SI(fs::file_size(iter_cur->path()))
					<< "</td>\n</tr>\n";
			}
		}
	}catch(std::exception & ex){
		LOGGER << "exception: " << ex.what();
		Proactor.disconnect(CI.connection_ID);
		return;
	}
	ss << "</table>\n</body>\n";
	std::string header = create_header(ss.str().size());
	network::buffer B;
	B.append(header).append(ss.str());
	Proactor.write(CI.connection_ID, B);
	Proactor.disconnect_on_empty(CI.connection_ID);
}
