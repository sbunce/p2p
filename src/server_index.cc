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
{
	//create the share directory if it doesn't exist
	boost::filesystem::create_directory(global::SERVER_SHARE_DIRECTORY);

	indexing = false;
	stop_thread = false;
	threads = 0;
}

int server_index::index_share_recurse(const std::string directory_name)
{
	namespace fs = boost::filesystem;

	fs::path full_path = fs::system_complete(fs::path(directory_name, fs::native));

	if(!fs::exists(full_path)){
#ifdef DEBUG
		std::cout << "error: fileIndex::index_share(): can't locate " << full_path.string() << "\n";
#endif
		return -1;
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
				}
				else{
					//determine size
					fs::path file_path = fs::system_complete(fs::path(directory_name + directory_iter->leaf(), fs::native));
					DB_Access.share_add_entry(fs::file_size(file_path), file_path.string());
				}
			}
			catch(std::exception & ex){
#ifdef DEBUG
				std::cout << "error: server_index::index_share_recurse(): file " << directory_iter->leaf() << " caused exception " << ex.what() << "\n";
#endif
			}
		}
	}
	else{
#ifdef DEBUG
		std::cout << "error: fileIndex::index_share(): index points to file when it should point at directory\n";    
#endif
	}

	return 0;
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
	if(threads != 0){
		std::cout << "fatal error: server_index::start(): cannot start multiple indexing threads\n";
		exit(1);
	}

	boost::thread T(boost::bind(&server_index::index_share, this));
}

void server_index::stop()
{
	stop_thread = true;

	while(threads){
		usleep(global::SPINLOCK_TIME);
	}
}
