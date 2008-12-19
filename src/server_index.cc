#include "server_index.h"

server_index::server_index():
	indexing(false),
	byte_count(0)
{
	boost::filesystem::create_directory(global::SHARE_DIRECTORY);
	indexing_thread = boost::thread(boost::bind(&server_index::index_share, this));
}

server_index::~server_index()
{
	//Hash_Tree.stop(); //force hash tree generation to terminate
	indexing_thread.interrupt();
	indexing_thread.join();
}

void server_index::generate_hash(const boost::filesystem::path file_path)
{
	namespace fs = boost::filesystem;

	std::string root_hash;
	boost::uint64_t existing_size;
	if(DB_Share.lookup_path(file_path.string(), root_hash, existing_size)){
		try{ //fs::file_size() might throw
			//database entry exists, make sure there is a corresponding hash tree file
			fs::path hash_path = fs::system_complete(fs::path(global::HASH_DIRECTORY + root_hash, fs::native));
			if(fs::exists(hash_path)){
				if(existing_size == fs::file_size(file_path)){
					//no need to generate hash tree
					return;
				}else{
					//file changed size, rehash it
					DB_Share.delete_hash(root_hash);
				}
			}else{
				//hash tree is missing
				DB_Share.delete_hash(root_hash);
			}
		}catch(std::exception & ex){
			logger::debug(LOGGER_P1, "caught ", ex.what());
			return;
		}
	}

	indexing = true;

	//create hash tree
	if(Hash_Tree.create(file_path.string(), root_hash)){
		//make sure user didn't move the file while it was hashing
		if(fs::exists(file_path)){
			//file still present
			DB_Share.add_entry(root_hash, fs::file_size(file_path), file_path.string());
		}else{
			//file missing delete hash tree
			fs::path path = fs::system_complete(fs::path(global::HASH_DIRECTORY + root_hash, fs::native));
			fs::remove(path);
		}
	}
}

void server_index::scan_hashes()
{
	namespace fs = boost::filesystem;

	fs::path full_path = fs::system_complete(fs::path(global::HASH_DIRECTORY, fs::native));
	fs::directory_iterator end_iter;
	for(fs::directory_iterator directory_iter(full_path); directory_iter != end_iter; directory_iter++){
		portable_sleep::ms(5); //make it so directory scans don't hog all CPU
		try{
			if(fs::is_directory(*directory_iter)){
				logger::debug(LOGGER_P1,"found directory: ",directory_iter->path().filename(),", in hash directory");
			}else{
				boost::this_thread::interruption_point();
				if(directory_iter->path().filename().length() != sha::HEX_HASH_SIZE){
					//file not a hash tree, it may be "upside_down" or "rightside_up" temporary files
					continue;
				}

				if(!client_server_bridge::is_downloading(directory_iter->path().filename())
					&& !DB_Share.lookup_hash(directory_iter->path().filename()))
				{
					std::remove((global::HASH_DIRECTORY+directory_iter->path().filename()).c_str());
				}
			}
		}catch(std::exception & ex){
			logger::debug(LOGGER_P1,"when trying to read file ",directory_iter->path().filename()," caught exception ",ex.what());
		}
	}
	return;
}

void server_index::scan_share(std::string directory_name)
{
	namespace fs = boost::filesystem;
	boost::this_thread::interruption_point();
	fs::path full_path = fs::system_complete(fs::path(directory_name, fs::native));
	if(!fs::exists(full_path)){
		logger::debug(LOGGER_P1,"can't locate ", full_path.string());
		return;
	}

	if(fs::is_directory(full_path)){
		fs::directory_iterator end_iter;
		for(fs::directory_iterator directory_iter(full_path); directory_iter != end_iter; directory_iter++){
			boost::this_thread::interruption_point();
			portable_sleep::ms(5); //make it so directory scans don't hog all CPU
			try{
				if(fs::is_directory(*directory_iter)){
					//recurse to new directory
					std::string sub_directory;
					sub_directory = directory_name + directory_iter->path().filename() + "/";
					scan_share(sub_directory);
				}else{
					fs::path path = fs::system_complete(fs::path(directory_name + directory_iter->path().filename(), fs::native));
					generate_hash(path);
				}
			}catch(std::exception & ex){
				logger::debug(LOGGER_P1,"when trying to read file ",directory_iter->path().filename()," caught exception ",ex.what());
			}
		}
	}else{
		logger::debug(LOGGER_P1,"index location is a file when it needs to be a directory");
		exit(1);
	}
	return;
}

bool server_index::is_indexing()
{
	return indexing;
}

void server_index::index_share()
{
	/*
	If hashes aren't in the DB or if they're not downloading they're deleted. This
	sleep gives downloads a chance to start before their associated hashes are
	deleted.
	*/
	portable_sleep::ms(5000);

	while(true){
		boost::this_thread::interruption_point();

		//remove hashes with no corresponding entry in the database
		scan_hashes();

		//remove share entries for files that no longer exist
		DB_Share.remove_missing(global::SHARE_DIRECTORY);

		//create hashes for all files in shares
		scan_share(global::SHARE_DIRECTORY);

		indexing = false;

		//wait between share scans and checks
		portable_sleep::ms(1000);
	}
}
