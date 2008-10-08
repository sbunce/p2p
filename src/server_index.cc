#include "server_index.h"

server_index::server_index():
	indexing(false),
	stop_thread(false),
	change_share(false),
	threads(0)
{
	boost::filesystem::create_directory(global::SHARE_DIRECTORY);
	boost::thread T(boost::bind(&server_index::index_share, this));
}

server_index::~server_index()
{
	stop_thread = true;
	Hash_Tree.stop(); //force hash tree generation to terminate
	while(threads){
		portable_sleep::yield();
	}
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
	std::string hash;
	if(!Hash_Tree.create_hash_tree(file_path.string(), hash)){
		return;
	}

	//make sure user didn't move the file while it was hashing
	if(file_exists(file_path.string())){
		//file still present
		DB_Share.add_entry(hash, fs::file_size(file_path), file_path.string());
	}else{
		//file missing delete hash tree
		fs::path path = fs::system_complete(fs::path(global::HASH_DIRECTORY+hash, fs::native));
		fs::remove(path);
	}
}

void server_index::scan_hashes()
{
	namespace fs = boost::filesystem;
	fs::path full_path = fs::system_complete(fs::path(global::HASH_DIRECTORY, fs::native));
	fs::directory_iterator end_iter;
	for(fs::directory_iterator directory_iter(full_path); directory_iter != end_iter; directory_iter++){
		portable_sleep::yield(); //make it so directory scans don't hog all CPU
		try{
			if(fs::is_directory(*directory_iter)){
				logger::debug(LOGGER_P1,"found directory: ",directory_iter->path().leaf(),", in hash directory");
			}else{
				if(directory_iter->path().leaf().length() != sha::HEX_HASH_LENGTH){
					//file not a hash tree, it may be "upside_down" or "rightside_up" temporary files
					continue;
				}

				if(client_server_bridge::is_downloading(directory_iter->path().leaf()) == client_server_bridge::NOT_DOWNLOADING && !DB_Share.hash_exists(directory_iter->path().leaf())){
					std::remove((global::HASH_DIRECTORY+directory_iter->path().leaf()).c_str());
				}

				if(stop_thread){
					return;
				}
			}
		}
		catch(std::exception & ex){
			logger::debug(LOGGER_P1,"when trying to read file ",directory_iter->path().leaf()," caught exception ",ex.what());
		}
	}

	return;
}

void server_index::scan_share(std::string directory_name)
{
	namespace fs = boost::filesystem;

	if(stop_thread){
		return;
	}

	fs::path full_path = fs::system_complete(fs::path(directory_name, fs::native));

	if(!fs::exists(full_path)){
		logger::debug(LOGGER_P1,"can't locate ", full_path.string());
		return;
	}

	if(fs::is_directory(full_path)){
		fs::directory_iterator end_iter;
		for(fs::directory_iterator directory_iter(full_path); directory_iter != end_iter; directory_iter++){
			portable_sleep::yield(); //make it so directory scans don't hog all CPU
			try{
				if(fs::is_directory(*directory_iter)){
					//recurse to new directory
					std::string sub_directory;
					sub_directory = directory_name + directory_iter->path().leaf() + "/";
					scan_share(sub_directory);
				}else{
					//determine if a hash tree needs to be generated
					fs::path file_path = fs::system_complete(fs::path(directory_name + directory_iter->path().leaf(), fs::native));
					std::string existing_hash;
					boost::uint64_t existing_size;
					if(DB_Share.lookup_path(file_path.string(), existing_hash, existing_size)){
						//database entry exists, make sure there is a corresponding hash tree file
						std::fstream fin((global::HASH_DIRECTORY+existing_hash).c_str(), std::ios::in);
						if(!fin.is_open()){
							//hash tree is missing
							indexing = true;
							DB_Share.delete_hash(existing_hash); //delete the entry
							generate_hash(file_path);            //regenerate hash for file
						}
						fin.close();

						/*
						Regenerate the hash if the file has changed size. The most
						common cause of this is that the file was hashed while it was
						copying.
						*/
						if(existing_size != fs::file_size(file_path)){
							indexing = true;
							DB_Share.delete_hash(existing_hash);
							generate_hash(file_path);
						}
					}else{
						//hash tree doesn't yet exist, generate one
						indexing = true;
						generate_hash(file_path);
					}

					if(stop_thread){
						return;
					}
				}
			}
			catch(std::exception & ex){
				logger::debug(LOGGER_P1,"when trying to read file ",directory_iter->path().leaf()," caught exception ",ex.what());
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
	/*
	If hashes aren't in the DB or if they're not downloading they're deleted. This
	sleep gives downloads a chance to start before their associated hashes are
	deleted.
	*/
	portable_sleep::ms(1000);

	++threads;
	indexing = false;
	while(true){
		if(stop_thread){
			break;
		}

		//remove hashes with no corresponding entry in the database
		scan_hashes();

		//remove share entries for files that no longer exist
		DB_Share.remove_missing(global::SHARE_DIRECTORY);

		//create hashes for all files in shares
		scan_share(global::SHARE_DIRECTORY);

		indexing = false;
		change_share = false;

		//wait 1 second between share scans and checks
		portable_sleep::ms(1000);
	}
	--threads;
}
