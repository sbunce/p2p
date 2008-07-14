#include "server_index.h"

server_index::server_index():
	indexing(false),
	stop_thread(false),
	change_share(false),
	threads(0)
{
	boost::filesystem::create_directory(global::SHARE_DIRECTORY);
	share_directory = DB_Server_Preferences.get_share_directory();
	boost::thread T(boost::bind(&server_index::index_share, this));
}

server_index::~server_index()
{
	stop_thread = true;
	Hash_Tree.stop(); //force hash tree generation to terminate
	while(threads){
		usleep(1);
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

void server_index::index_share_recurse(std::string directory_name)
{
	namespace fs = boost::filesystem;

	if(stop_thread || change_share){
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
			/*
			Directory scans happen frequently. The idea of putting a sleep here is
			to slow them down. Most of the time is spent hashing files if there is
			stuff needing to be hashed. After that is done there are constant
			checks for updated files. This makes those checks such that they don't
			contantly load the CPU.
			*/
			usleep(1);
			try{
				if(fs::is_directory(*directory_iter)){
					//recurse to new directory
					std::string sub_directory;
					sub_directory = directory_name + directory_iter->leaf() + "/";
					index_share_recurse(sub_directory);
				}else{
					//determine if a hash tree needs to be generated
					fs::path file_path = fs::system_complete(fs::path(directory_name + directory_iter->leaf(), fs::native));
					std::string existing_hash;
					uint64_t existing_size;
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

					if(stop_thread || change_share){
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
	indexing = false;
	std::string share_directory_tmp = share_directory;
	while(true){
		if(stop_thread){
			break;
		}

		if(share_directory_tmp != share_directory){
			//share directory changed
			DB_Share.clear();
		}

		{//begin lock scope
		boost::mutex::scoped_lock lock(SD_mutex);
		share_directory_tmp = share_directory;
		}//end lock scope

		//remove share entries for files that no longer exist
		DB_Share.remove_missing(share_directory_tmp);

		//create hashes
		index_share_recurse(share_directory_tmp);

		indexing = false;
		change_share = false;
		sleep(1);
	}
	--threads;
}

void server_index::set_share_directory(const std::string & directory)
{
	boost::mutex::scoped_lock lock(SD_mutex);
	share_directory = directory;
	change_share = true;
}
