#include "download_factory.h"

download_factory::download_factory()
{

}

bool download_factory::start_file(download_info & info, download *& Download, std::list<download_conn *> & servers)
{
	//make sure file isn't already downloading
	if(!info.resumed){
		if(!DB_Download.start_download(info)){
			return false;
		}
	}

	//get file path, stop if file not found
	std::string file_path;
	if(!DB_Download.get_file_path(info.hash, file_path)){
		return false;
	}

	//create an empty file for this download, if a file doesn't already exist
	std::fstream fin(file_path.c_str(), std::ios::in);
	if(fin.is_open()){
		fin.close();
	}else{
		std::fstream fout(file_path.c_str(), std::ios::out);
		fout.close();
	}

	//(global::P_BLS_SIZE - 1) because control size is 1 byte
	uint64_t last_block = info.size/(global::P_BLOCK_SIZE - 1);
	uint64_t last_block_size = info.size % (global::P_BLOCK_SIZE - 1) + 1;

	Download = new download_file(
		info.hash,
		info.name,
		file_path,
		info.size,
		info.latest_request,
		last_block,
		last_block_size
	);

	for(int x = 0; x < info.server_IP.size(); ++x){
		servers.push_back(new download_file_conn(Download, info.server_IP[x]));
	}

	return true;
}

bool download_factory::stop(download * Download_stop, download *& Download_start, std::list<download_conn *> & servers)
{
	if(typeid(*Download_stop) == typeid(download_hash_tree)){
//DEBUG, need to start a download_file here

		return true;
	}else if(typeid(*Download_stop) == typeid(download_file)){
		DB_Download.terminate_download(Download_stop->hash());
		delete Download_stop;
		return false;
	}
}
