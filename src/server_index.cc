#include "server_index.h"

server_index::server_index():
	indexing(false)
{
	boost::filesystem::create_directory(global::SHARE_DIRECTORY);
	indexing_thread = boost::thread(boost::bind(&server_index::index_share, this));
}

server_index::~server_index()
{
	indexing_thread.interrupt();
	indexing_thread.join();
}

void server_index::generate_hash_tree(const std::string & file_path)
{
	namespace fs = boost::filesystem;
	boost::uint64_t existing_size, current_size;

	try{
		current_size = fs::file_size(file_path);
	}catch(std::exception & ex){
		LOGGER << ex.what();
		return;
	}

	std::string hash;
	if(DB_Share.lookup_path(file_path, hash, existing_size)){
		if(existing_size == current_size){
			return;
		}else{
			DB_Share.delete_entry(hash, file_path);
		}
	}

	std::string root_hash;
	if(Hash_Tree.create(file_path, root_hash)){
		DB_Share.add_entry(root_hash, current_size, file_path);
	}
}

void server_index::scan_share(std::string directory_name)
{
	namespace fs = boost::filesystem;
	fs::path path = fs::system_complete(fs::path(directory_name, fs::native));
	if(!fs::exists(path)){
		LOGGER << "error opening share directory: " << path.string();
		return;
	}

	try{
		fs::recursive_directory_iterator iter_cur(path), iter_end;
		while(iter_cur != iter_end){
			boost::this_thread::interruption_point();
			portable_sleep::ms(10); //slow down directory scans
			if(!fs::is_directory(iter_cur->path())){
				generate_hash_tree(iter_cur->path().string());
			}
			++iter_cur;
		}
	}catch(std::exception & ex){
		LOGGER << ex.what();
		return;
	}
}

bool server_index::is_indexing()
{
	return indexing;
}

void server_index::index_share()
{
	while(client_server_bridge::server_index_is_blocked()){
		portable_sleep::ms(1000);
	}

	while(true){
		boost::this_thread::interruption_point();
		DB_Share.remove_missing(global::SHARE_DIRECTORY);
		scan_share(global::SHARE_DIRECTORY);
		indexing = false;
		portable_sleep::ms(1000);
	}
}
