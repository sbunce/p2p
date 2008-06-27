#include "download_factory.h"

download_factory::download_factory()
{

}

bool download_factory::start_hash(download_info & info, download *& Download, std::list<download_conn *> & servers, bool resumed)
{
	if(!DB_Download.start_download(info)){
		//download already exists in database
		if(!resumed){
			//download is resumed so it's normal that the download couldn't be added
			logger::debug(LOGGER_P1,"resuming download ",info.name);
			return false;
		}
	}
	Download = new download_hash_tree(info.hash, info.size, info.name);
	for(int x = 0; x < info.IP.size(); ++x){
		servers.push_back(new download_hash_tree_conn(Download, info.IP[x]));
	}
	return true;
}

download_file * download_factory::start_file(download_hash_tree * DHT, std::list<download_conn *> & servers)
{
	std::string file_path;
	if(!DB_Download.get_file_path(DHT->hash(), file_path)){
		logger::debug(LOGGER_P1,"download ",DHT->download_file_name()," not found in database");
	}
	download_file * Download = new download_file(
		DHT->hash(),
		DHT->download_file_name(),
		file_path,
		DHT->download_file_size()
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
		if(((download_hash_tree *)Download_Stop)->canceled() == true){
			//user cancelled download, don't trigger download_file start
			DB_Download.terminate_download(Download_Stop->hash());
			delete Download_Stop;
			return false;
		}else{
			//trigger download_file start
			Download_Start = start_file((download_hash_tree *)Download_Stop, servers);
			delete Download_Stop;
			return true;
		}
	}else if(typeid(*Download_Stop) == typeid(download_file)){
		DB_Download.terminate_download(Download_Stop->hash());
		delete Download_Stop;
		return false;
	}
}
