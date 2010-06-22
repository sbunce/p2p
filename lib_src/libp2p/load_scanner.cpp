#include "load_scanner.hpp"

load_scanner::load_scanner():
	Thread_Pool(this)
{
	Thread_Pool.enqueue(boost::bind(&load_scanner::scan, this));
}

load_scanner::~load_scanner()
{
	Thread_Pool.stop();
	Thread_Pool.clear();
}

void load_scanner::load(const boost::filesystem::path & path)
{
	//verify file has .p2p extension
	std::string path_str = path.string();
	boost::regex expr(".+\\.p2p");
	if(!boost::regex_match(path_str.begin(), path_str.end(), expr)){
		LOG << "foreign file \"" << path_str << "\"";
		if(boost::filesystem::exists(path::load_bad_dir() + path.filename())){
			//resolve naming conflict and move file
			unsigned num = 1;
			while(true){
				std::stringstream new_path;
				new_path << path::load_bad_dir() << path.filename() << "_" << num++;
				if(!boost::filesystem::exists(new_path.str())){
					boost::filesystem::rename(path, new_path.str());
					break;
				}
			}
		}else{
			//move file
			boost::filesystem::rename(path, path::load_bad_dir() + path.filename());
		}
		return;
	}

	//read file
	std::fstream fin(path_str.c_str(), std::ios::in);
	std::string buf;
	while(std::getline(fin, buf)){
		//min row size is hex hash + 1 char file name
		if(buf.size() < SHA1::hex_size + 1){
			LOG << "row < " << SHA1::hex_size + 1;
			continue;
		}
		//max row size is hex hash + 256 char file name
		if(buf.size() > SHA1::hex_size + 255){
			LOG << "row > " << SHA1::hex_size + 255;
			continue;
		}
		//verify hash contains only hex chars
		if(!convert::hex_validate(std::string(buf, 0, SHA1::hex_size))){
			LOG << "hash contains non-hex chars";
			continue;
		}
		//stop directory traversal
		if(buf.find("..") != std::string::npos){
			LOG << "\"..\" not allowed";
		}
		//everything checks out
		start(std::string(buf, 0, SHA1::hex_size), std::string(buf, SHA1::hex_size));
	}

	//remove loaded file
	boost::filesystem::remove(path);
}

void load_scanner::scan()
{
	try{
		for(boost::filesystem::directory_iterator it_cur(path::load_dir()),
			it_end; it_cur != it_end; ++it_cur)
		{
			if(!boost::filesystem::is_directory(it_cur->path())){
				load(it_cur->path());
			}
		}
	}catch(const std::exception & e){
		LOG << e.what();
	}
	Thread_Pool.enqueue(boost::bind(&load_scanner::scan, this), 1000);
}

void load_scanner::start(const std::string & hash, const std::string file_name)
{
	LOG << hash << " " << file_name;
}
