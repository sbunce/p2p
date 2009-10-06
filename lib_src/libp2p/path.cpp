#include "path.hpp"

boost::once_flag path::once_flag = BOOST_ONCE_INIT;

void path::create_required_directories()
{
	boost::call_once(init, once_flag);
	try{
		#ifndef _WIN32
		//main directory always in current directory on windows
		if(!program_directory().empty()){
			boost::filesystem::create_directory(program_directory());
		}
		#endif
		boost::filesystem::create_directory(download());
		boost::filesystem::create_directory(share());
		boost::filesystem::create_directory(temp());
	}catch(std::exception & ex){
		LOGGER << ex.what();
		LOGGER << "error: could not create required directory";
		exit(1);
	}
}

std::string path::database()
{
	boost::call_once(init, once_flag);
	return program_directory() + database_name();
}

std::string & path::database_name()
{
	static std::string * _database_name = new std::string();
	return *_database_name;
}

void path::init()
{
	//setup program_directory
	const char * temp = std::getenv("HOME"); //std::getenv is not thread safe
	if(temp != NULL){
		program_directory() = temp;
		//make sure path doesn't end in '/'
		if(!program_directory().empty() && program_directory()[program_directory().size()-1] == '/'){
			program_directory().erase(program_directory().size()-1);
		}
	}
	if(!program_directory().empty()){
		program_directory() += "/.p2p/";
	}

	//setup database_name
	database_name() = "DB";
}

std::string path::download()
{
	boost::call_once(init, once_flag);
	return program_directory() + "download/";
}

std::string path::hash_tree_temp()
{
	boost::call_once(init, once_flag);
	std::stringstream ss;
	ss << program_directory() << "temp/hash_tree_" << boost::this_thread::get_id();
	return ss.str();
}

std::string & path::program_directory()
{
	static std::string * _program_directory = new std::string();
	return *_program_directory;
}

void path::remove_temporary_hash_tree_files()
{
	boost::call_once(init, once_flag);
	namespace fs = boost::filesystem;
	boost::uint64_t size;
	fs::path path = fs::system_complete(fs::path(temp(), fs::native));
	if(fs::exists(path)){
		try{
			for(fs::directory_iterator iter_cur(path), iter_end; iter_cur != iter_end;
				++iter_cur)
			{
				if(iter_cur->path().filename().find("hash_tree_", 0) == 0){
					fs::remove(iter_cur->path());
				}
			}
		}catch(std::exception & ex){
			LOGGER << ex.what();
		}
	}
}

std::string path::share()
{
	boost::call_once(init, once_flag);
	return program_directory() + "share/";
}

std::string path::temp()
{
	boost::call_once(init, once_flag);
	return program_directory() + "temp/";
}

void path::unit_test_override(const std::string & DB_path)
{
	boost::call_once(init, once_flag);
	program_directory() = "";
	database_name() = DB_path;
}
