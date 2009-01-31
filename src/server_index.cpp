#include "server_index.hpp"

server_index::server_index()
:
	indexing(false),
	generate_hash_tree_disabled(true)
{
	boost::filesystem::create_directory(global::SHARE_DIRECTORY);
	indexing_thread = boost::thread(boost::bind(&server_index::main_loop, this));
}

server_index::~server_index()
{
	indexing_thread.interrupt();
	indexing_thread.join();
}

void server_index::check_path(const std::string & path)
{
	namespace fs = boost::filesystem;
	std::deque<std::string> tokens;
	tokenize_path(path, tokens);
	bool is_file;
	boost::uint64_t file_size;
	if(fs::is_directory(path)){
		is_file = false;
	}else{
		try{
			file_size = fs::file_size(path);
		}catch(std::exception & ex){
			LOGGER << ex.what();
			return;
		}
		is_file = true;
	}
	check_path_recurse(is_file, file_size, Root, tokens, path);
}

void server_index::check_path_recurse(const bool & is_file, const boost::uint64_t & file_size,
	std::map<std::string, directory_contents> & Directory, std::deque<std::string> & tokens, const std::string & path)
{
	if(tokens.size() == 0){
		return;
	}else{
		//add directory if it doesn't exist
		std::pair<std::map<std::string, directory_contents>::iterator, bool> ret_dir;
		ret_dir = Directory.insert(std::make_pair(tokens.front(), directory_contents()));
		tokens.pop_front();
		if(is_file && tokens.size() == 1){
			//last token is file name
			std::pair<std::map<std::string, boost::uint64_t>::iterator, bool> ret_file;
			ret_file = ret_dir.first->second.File.insert(std::make_pair(tokens.front(), file_size));
			if(ret_file.second){
				//new file added to tree
				generate_hash_tree(path);
			}
		}else{
			check_path_recurse(is_file, file_size, ret_dir.first->second.Directory, tokens, path);
		}
	}
}

void server_index::check_missing()
{
	check_missing_recurse_1(Root, "/");
}

/*
This function is mutually recursive with remove_missing_recurse_2. This function
calls remove_missing_recurse_2 on all directory_contents elements,
remove_missing_recurse_2 processes all files in the directory, and calls
remove_missing_recurse_1 on all directories within the directory.
*/
void server_index::check_missing_recurse_1(
	std::map<std::string, directory_contents> & Directory, std::string current_directory)
{
	std::map<std::string, directory_contents>::iterator iter_cur, iter_end;
	iter_cur = Directory.begin();
	iter_end = Directory.end();
	std::vector<std::string> remove;
	while(iter_cur != iter_end){
		portable_sleep::ms(1000 / global::SHARE_SCAN_RATE);
		if(check_missing_recurse_2(iter_cur->second, current_directory + iter_cur->first + "/")){
			//directory passed to check_missing_recurse_2 is empty, remove it
			Directory.erase(iter_cur++);
		}else{
			++iter_cur;
		}
	}	
}

bool server_index::check_missing_recurse_2(directory_contents & DC, std::string current_directory)
{
	namespace fs = boost::filesystem;
	std::map<std::string, boost::uint64_t>::iterator iter_cur, iter_end;
	iter_cur = DC.File.begin();
	iter_end = DC.File.end();
	while(iter_cur != iter_end){
		portable_sleep::ms(1000 / global::SHARE_SCAN_RATE);
		std::string path = current_directory + iter_cur->first;
		if(!fs::exists(path)){
			//file is missing, remove it from DB
			DB_Share.delete_entry(path);
			DC.File.erase(iter_cur++);
		}else{
			//file exists, make sure it's the same size
			boost::uint64_t file_size;
			try{
				file_size = fs::file_size(path);
			}catch(std::exception & ex){
				LOGGER << ex.what();
				return false; //read error but don't remove yet
			}

			if(file_size != iter_cur->second){
				//file changed size, rehash
				DB_Share.delete_entry(path);
				iter_cur->second = file_size;
				generate_hash_tree(path);
			}
			++iter_cur;
		}
	}
	check_missing_recurse_1(DC.Directory, current_directory);
	return DC.Directory.empty() && DC.File.empty();
}

void server_index::generate_hash_tree(const std::string & file_path)
{
	namespace fs = boost::filesystem;

	if(generate_hash_tree_disabled){
		return;
	}

	boost::uint64_t file_size;
	try{
		file_size = fs::file_size(file_path);
	}catch(std::exception & ex){
		LOGGER << ex.what();
		return;
	}

	indexing = true;
	std::string root_hash;
	if(Hash_Tree.create(file_path, root_hash)){
		DB_Share.add_entry(root_hash, file_size, file_path);
	}
}

bool server_index::is_indexing()
{
	return indexing;
}

int server_index::path_call_back(int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0]);
	check_path(response[0]);
	return 0;
}

void server_index::main_loop()
{
	while(client_server_bridge::server_index_is_blocked()){
		portable_sleep::ms(1000);
	}

	DB.query("SELECT path FROM share", this, &server_index::path_call_back);
	generate_hash_tree_disabled = false;

	while(true){
		boost::this_thread::interruption_point();
		scan_share(global::SHARE_DIRECTORY);
		indexing = false;
		check_missing();

		//1 second sleep between scans, needed for when share empty
		portable_sleep::ms(1000);
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
			portable_sleep::ms(1000 / global::SHARE_SCAN_RATE);
			check_path(iter_cur->path().string());
			++iter_cur;
		}
	}catch(std::exception & ex){
		LOGGER << ex.what();
		return;
	}
}

void server_index::tokenize_path(const std::string & path, std::deque<std::string> & tokens)
{
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> char_sep("/");
	tokenizer path_tokens(path, char_sep);
	tokenizer::iterator iter_cur, iter_end;
	iter_cur = path_tokens.begin();
	iter_end = path_tokens.end();
	while(iter_cur != iter_end){
		tokens.push_back(*iter_cur);
		++iter_cur;
	}
}
