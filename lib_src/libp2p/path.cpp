#include "path.hpp"

//BEGIN static_wrap
boost::once_flag path::static_wrap::once_flag = BOOST_ONCE_INIT;

path::static_wrap::static_wrap()
{
	//thread safe initialization of static data members
	boost::call_once(init, once_flag);
}

std::string & path::static_wrap::database_name()
{
	return _database_name();
}

std::string & path::static_wrap::program_directory()
{
	return _program_directory();
}

void path::static_wrap::init()
{
	//make sure all statics initialized
	_database_name();
	_program_directory();

	//setup program_directory
	const char * temp = std::getenv("HOME"); //std::getenv is not thread safe
	if(temp != NULL){
		_program_directory() = temp;
		//make sure path doesn't end in '/'
		if(!_program_directory().empty() && _program_directory()[_program_directory().size()-1] == '/'){
			_program_directory().erase(_program_directory().size()-1);
		}
	}
	if(!_program_directory().empty()){
		_program_directory() += "/.p2p/";
	}

	//setup database_name
	_database_name() = "DB";
}

std::string & path::static_wrap::_database_name()
{
	static std::string DN;
	return DN;
}

std::string & path::static_wrap::_program_directory()
{
	static std::string PD;
	return PD;
}
//END static_wrap

void path::create_required_directories()
{
	static_wrap Static_Wrap;
	try{
		#ifndef _WIN32
		//main directory always in current directory on windows
		if(!Static_Wrap.program_directory().empty()){
			boost::filesystem::create_directory(Static_Wrap.program_directory());
		}
		#endif
		boost::filesystem::create_directory(download());
		boost::filesystem::create_directory(share());
		boost::filesystem::create_directory(temp());
	}catch(std::exception & ex){
		LOGGER(logger::fatal) << ex.what();
		exit(1);
	}
}

std::string path::database()
{
	static_wrap Static_Wrap;
	return Static_Wrap.program_directory() + Static_Wrap.database_name();
}

std::string path::download()
{
	static_wrap Static_Wrap;
	return Static_Wrap.program_directory() + "download/";
}

std::string path::hash_tree_temp()
{
	static_wrap Static_Wrap;
	std::stringstream ss;
	ss << Static_Wrap.program_directory() << "temp/hash_tree_" << boost::this_thread::get_id();
	return ss.str();
}

void path::override_database_name(const std::string & DB_name)
{
	static_wrap Static_Wrap;
	Static_Wrap.database_name() = DB_name;
}

void path::override_program_directory(const std::string & path)
{
	static_wrap Static_Wrap;
	Static_Wrap.program_directory() = path;
}

void path::remove_temporary_hash_tree_files()
{
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
			LOGGER(logger::error) << ex.what();
		}
	}
}

std::string path::share()
{
	static_wrap Static_Wrap;
	return Static_Wrap.program_directory() + "share/";
}

std::string path::temp()
{
	static_wrap Static_Wrap;
	return Static_Wrap.program_directory() + "temp/";
}
