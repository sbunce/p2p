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

static bool file_exists(const std::string & path)
{
	std::fstream fin(path.c_str(), std::ios::in);
	return fin.is_open();
}

void server_index::generate_hash(const boost::filesystem::path & file_path)
{
	namespace fs = boost::filesystem;

	//no entry for file exists, create hash tree
	std::string hash = Hash_Tree.create_hash_tree(file_path.string());

	//make sure user didn't move the file while it was hashing
	if(file_exists(file_path.string())){
		//file still present
		DB_Access.share_add_entry(hash, fs::file_size(file_path), file_path.string());
	}else{
		//file missing delete hash tree
		fs::path path = fs::system_complete(fs::path(global::HASH_DIRECTORY+hash, fs::native));
		fs::remove(path);
	}
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

					std::string existing_hash;
					if(!DB_Access.share_file_exists(existing_hash, file_path.string())){
						generate_hash(file_path);
					}else{
						//entry exists, make sure hash file exists
						std::fstream fin((global::HASH_DIRECTORY+existing_hash).c_str(), std::ios::in);
						if(!fin.is_open()){
							//hash file doesn't exist, delete entry from DB and generate new hash tree
							DB_Access.share_delete_hash(existing_hash);
							generate_hash(file_path);
						}
					}

					if(stop_thread){
						return;
					}
				}
			}
			catch(std::exception & ex){
				logger::debug(LOGGER_P1,"when trying to read file ",directory_iter->leaf()," caught exception ",ex.what());
			}
		}
	}else{
		logger::debug(LOGGER_P1,"index location is a file when it needs to be a directory");
		assert(false);
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
