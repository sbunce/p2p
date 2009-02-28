#include "download_factory.hpp"

download_factory::download_factory()
{

}

boost::shared_ptr<download> download_factory::start(download_info info, std::vector<download_connection> & servers)
{
	if(block_arbiter::instance().is_downloading(info.hash)){
		LOGGER << "file '" << info.name << "' already downloading";
		return boost::shared_ptr<download>();
	}
	if(DB_Share.lookup_hash(info.hash)){
		LOGGER << "file '" << info.name << "' already exists in share";
		return boost::shared_ptr<download>();
	}

	boost::shared_ptr<download> Download;
	if(DB_Download.lookup_hash(info.hash)){
		//download being resumed
		database::table::hash::state State = DB_Hash.get_state(info.hash, hash_tree::tree_info::file_size_to_tree_size(info.size));
		if(State == database::table::hash::DOWNLOADING){
			Download = start_hash_tree(info, servers);
		}else if(State == database::table::hash::COMPLETE){
			Download = start_file(info, servers);
		}else{
			LOGGER << "unknown state: " << State;
			exit(1);
		}
	}else{
		//new download being started
		if(DB_Download.start(info)){
			Download = start_hash_tree(info, servers);
		}
	}
	return Download;
}

boost::shared_ptr<download> download_factory::start_file(const download_info & info, std::vector<download_connection> & servers)
{
	boost::shared_ptr<download> Download(new download_file(info));
	for(int x=0; x < info.IP.size(); ++x){
		servers.push_back(download_connection(Download, info.IP[x]));
	}
	return Download;
}

boost::shared_ptr<download> download_factory::start_hash_tree(const download_info & info, std::vector<download_connection> & servers)
{
	boost::shared_ptr<download> Download(new download_hash_tree(info));
	for(int x=0; x < info.IP.size(); ++x){
		servers.push_back(download_connection(Download, info.IP[x]));
	}
	return Download;
}

boost::shared_ptr<download> download_factory::stop(boost::shared_ptr<download> Download_Stop, std::vector<download_connection> & servers)
{
	boost::shared_ptr<download> Download_Start;
	if(typeid(*Download_Stop) == typeid(download_hash_tree)){
		if(!Download_Stop->is_cancelled()){
			Download_Start = start_file(Download_Stop->get_download_info(), servers);
		}
	}else if(typeid(*Download_Stop) == typeid(download_file)){
		download_info Download_Info = Download_Stop->get_download_info();
		if(!Download_Stop->is_cancelled()){
			DB_Share.add_entry(Download_Info.hash, Download_Info.size, Download_Info.file_path);
			DB_Download.complete(Download_Info.hash, Download_Info.size);
			share_index::add_path(Download_Info.file_path);
		}
	}else{
		LOGGER << "unrecognized download type";
		exit(1);
	}
	return Download_Start;
}
