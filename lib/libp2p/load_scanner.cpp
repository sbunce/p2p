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

void load_scanner::load(const boost::filesystem::path & load_file)
{
	//verify file has .p2p extension
	std::string path_str = load_file.string();
	boost::regex expr(".+\\.p2p");
	if(!boost::regex_match(path_str.begin(), path_str.end(), expr)){
		LOG << "foreign file \"" << path_str << "\"";
		if(boost::filesystem::exists(path::load_bad_dir() + load_file.filename())){
			//resolve naming conflict and move foreign file
			unsigned num = 1;
			while(true){
				std::stringstream new_path;
				new_path << path::load_bad_dir() << load_file.filename() << "_" << num++;
				if(!boost::filesystem::exists(new_path.str())){
					boost::filesystem::rename(load_file, new_path.str());
					break;
				}
			}
		}else{
			//move foreign file
			boost::filesystem::rename(load_file, path::load_bad_dir() + load_file.filename());
		}
		return;
	}

	//std::map<path_upper, path>, used to check for case conflicts
	std::map<boost::filesystem::path, boost::filesystem::path> path_map;

	//used to insure only one top level directory or file created
	boost::filesystem::path first_path;

	std::fstream fin(path_str.c_str(), std::ios::in);
	std::string buf;
	while(std::getline(fin, buf)){
		//min row size is hex hash + 1 char file name
		if(buf.size() < SHA1::hex_size + 1){
			LOG << "error, row size less than minimum";
			goto end;
		}
		std::string hash = buf.substr(0, SHA1::hex_size);
		std::string tmp = buf.substr(SHA1::hex_size);
		boost::filesystem::path path(tmp);
		boost::to_upper(tmp);
		boost::filesystem::path path_upper;

		//only allow creation of one top level directory or file
		if(first_path.empty()){
			first_path = path;
		}else{
			if(*path.begin() != *first_path.begin()){
				LOG << "error, multiple top level files";
				goto end;
			}
		}
/*
		//rule 2: directory and file names < 255 chars
		for(boost::filesystem::path::iterator it_cur = path.begin(),
			it_end = path.end(); it_cur != it_end; ++it_cur)
		{
			if(it_cur->size() > 255){
				LOG << "error, directory or file name > 255 chars";
				goto end;
			}
		}

		//rule 3: file extensions are lower case
		std::string::size_type pos = path.filename().find(".");
		if(pos != std::string::npos){
			std::string file_extension = path.filename().substr(pos);
			if(!boost::all(file_extension, boost::is_lower())){
				LOG << "error, upper case file extension";
				goto end;
			}
		}

		//verify hash contains only hex chars
		if(!convert::hex_validate(hash)){
			LOG << "hash contains non-hex chars";
			goto end;
		}

		//verify there is no directory traversal
		if(path.string().find("..") != std::string::npos){
			LOG << "\"..\" not allowed";
			goto end;
		}

		//verify path is unique
		std::pair<std::set<boost::filesystem::path>::iterator, bool>
			ret = path_set.insert(path);
		if(!ret.second){
			LOG << "duplicate path";
			goto end;
		}
*/
		LOG << hash << " " << path;
	}

	//start downloads

	end:
	boost::filesystem::remove(load_file);
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
