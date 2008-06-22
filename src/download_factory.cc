#include "download_factory.h"

download_factory::download_factory()
{

}

bool download_factory::start_hash(download_info & info, download *& Download, std::list<download_conn *> & servers)
{
	if(!DB_Download.start_download(info)){
		return false;
	}

	/*
	Make sure there is an empty file for the download. This makes it so the download
	is not automatically stopped if the program is started and no blocks have been
	downloaded.
	*/
	std::fstream fin((global::HASH_DIRECTORY+info.hash).c_str(), std::ios::in);
	if(fin.is_open()){
		fin.close();
	}else{
		std::fstream fout((global::HASH_DIRECTORY+info.hash).c_str(), std::ios::out);
		fout.close();
	}

	Download = new download_hash_tree(info.hash, info.size, info.name);

	for(int x = 0; x < info.IP.size(); ++x){
		servers.push_back(new download_hash_tree_conn(Download, info.IP[x]));
	}
	return true;
}

download_file * download_factory::start_file(download_hash_tree * DHT, std::list<download_conn *> & servers)
{
	//get file path, stop if file not found
	std::string file_path;
	if(!DB_Download.get_file_path(DHT->hash(), file_path)){
		std::cout << "fatal error starting download_file\n";
	}

	//create an empty file for this download, if a file doesn't already exist
	std::fstream fin(file_path.c_str(), std::ios::in);
	if(!fin.is_open()){
		std::fstream fout(file_path.c_str(), std::ios::out);
		fout.close();
	}

	uint64_t last_block = DHT->file_size/(global::P_BLOCK_SIZE - 1);
	uint64_t last_block_size = DHT->file_size % (global::P_BLOCK_SIZE - 1) + 1;

	download_file * Download = new download_file(
		DHT->hash(),
		DHT->file_name,
		file_path,
		DHT->file_size,
		0,              //DEBUG, latest request, needs to be fixed
		last_block,
		last_block_size
	);

	std::vector<std::string> IP;
	DB_Search.get_servers(DHT->hash(), IP);
	for(int x = 0; x < IP.size(); ++x){
		servers.push_back(new download_file_conn(Download, IP[x]));
	}

	return Download;
}

bool download_factory::stop(download * Download_Stop, download *& Download_Start, std::list<download_conn *> & servers)
{
	if(typeid(*Download_Stop) == typeid(download_hash_tree)){
		Download_Start = start_file((download_hash_tree *)Download_Stop, servers);
		delete Download_Stop;
		return true;
	}else if(typeid(*Download_Stop) == typeid(download_file)){
		DB_Download.terminate_download(Download_Stop->hash());
		delete Download_Stop;
		return false;
	}
}
