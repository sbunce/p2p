//boost
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

//std
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "server_index.h"

server_index::server_index()
: indexing(false), stop_thread(false), threads(0)
{
	//create the share directory if it doesn't exist
	boost::filesystem::create_directory(global::SERVER_SHARE_DIRECTORY);
}

void server_index::index_share_recurse(std::string directory_name)
{
	if(stop_thread){
		return;
	}

	namespace fs = boost::filesystem;
	fs::path full_path = fs::system_complete(fs::path(directory_name, fs::native));

	if(!fs::exists(full_path)){
		logger::debug(LOGGER_P1,"can't locate ", full_path.string());
		return;
	}

	if(fs::is_directory(full_path)){
		fs::directory_iterator end_iter;
		for(fs::directory_iterator directory_iter(full_path); directory_iter != end_iter; directory_iter++){
			try{
				if(fs::is_directory(*directory_iter)){
					//recurse to new directory
					std::string sub_directory;
					sub_directory = directory_name + directory_iter->leaf() + "/";
					index_share_recurse(sub_directory);
				}else{
					//determine size
					fs::path file_path = fs::system_complete(fs::path(directory_name + directory_iter->leaf(), fs::native));
					std::string hash = Hash_Tree.create_hash_tree(file_path.string());
					if(stop_thread){
						return;
					}

					DB_Access.share_add_entry(hash, fs::file_size(file_path), file_path.string());
				}
			}
			catch(std::exception & ex){
				logger::debug(LOGGER_P1,"when trying to read file ",directory_iter->leaf()," caught exception ",ex.what());
			}
		}
	}else{
		logger::debug(LOGGER_P1,"index location is a file when it needs to be a directory");
	}

	return;
}

bool server_index::is_indexing()
{
	return indexing;
}

void server_index::index_share()
{
	++threads;

	//index when thread started
	indexing = true;
	DB_Access.share_remove_missing();
	index_share_recurse(global::SERVER_SHARE_DIRECTORY);
	indexing = false;

	int seconds_slept = 0;
	while(true){
		if(stop_thread){
			break;
		}

		sleep(1);
		++seconds_slept;
		if(seconds_slept > global::SHARE_REFRESH){
			seconds_slept = 0;
			indexing = true;
			DB_Access.share_remove_missing();
			index_share_recurse(global::SERVER_SHARE_DIRECTORY);
			indexing = false;
		}
	}

	--threads;
}

void server_index::start()
{
	assert(threads == 0);
	boost::thread T(boost::bind(&server_index::index_share, this));
}

void server_index::stop()
{
	stop_thread = true;
	Hash_Tree.stop();

	while(threads){
		usleep(global::SPINLOCK_TIME);
	}
}
