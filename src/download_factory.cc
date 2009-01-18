#include "download_factory.h"

download_factory::download_factory()
: DB_Hash(DB)
{

}

bool download_factory::start(download_info info, download *& Download, std::list<download_connection> & servers)
{
	if(client_buffer::is_downloading(info.hash)){
		LOGGER << "file '" << info.name << "' already downloading";
		return false;
	}

	if(DB_Share.lookup_hash(info.hash)){
		LOGGER << "file '" << info.name << "' already exists in share";
		return false;
	}

	if(DB_Download.lookup_hash(info.hash, info.key)){
		//download being resumed
		DB_hash::state State = DB_Hash.get_state(info.key);
		if(State == DB_hash::DOWNLOADING){
			Download = start_hash_tree(info, servers);
		}else if(State == DB_hash::COMPLETE){
			Download = start_file(info, servers);
		}else{
			LOGGER << "unknown state: " << State;
			exit(1);
		}
	}else{
		//new download being started
		if(!DB_Download.start(info)){
			return false;
		}
		Download = start_hash_tree(info, servers);
	}
	return true;
}

download * download_factory::start_file(const download_info & info, std::list<download_connection> & servers)
{
	download * Download = new download_file(info);
	for(int x=0; x < info.IP.size(); ++x){
		servers.push_back(download_connection(Download, info.IP[x]));
	}
	return Download;
}

download * download_factory::start_hash_tree(const download_info & info, std::list<download_connection> & servers)
{
	download * Download = new download_hash_tree(info);
	for(int x=0; x < info.IP.size(); ++x){
		servers.push_back(download_connection(Download, info.IP[x]));
	}
	return Download;
}

bool download_factory::stop(download * Download_Stop, download *& Download_Start, std::list<download_connection> & servers)
{
	if(typeid(*Download_Stop) == typeid(download_hash_tree)){
		if(((download_hash_tree *)Download_Stop)->canceled() == true){
			//user cancelled download, don't trigger download_file start
			delete Download_Stop;
			return false;
		}else{
			//trigger download_file start
			Download_Start = start_file(((download_hash_tree *)Download_Stop)->get_download_info(), servers);
			delete Download_Stop;
			return true;
		}
	}else if(typeid(*Download_Stop) == typeid(download_file)){
		delete Download_Stop;
		return false;
	}else{
		LOGGER << "unrecognized download type";
		exit(1);
	}
}
