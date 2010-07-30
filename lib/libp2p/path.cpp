#include "path.hpp"

//BEGIN static_wrap
boost::once_flag path::static_wrap::once_flag = BOOST_ONCE_INIT;

path::static_wrap::static_objects::static_objects()
{
	//std::getenv is not thread safe
	const char * tmp = std::getenv("HOME");
	if(tmp != NULL){
		program_dir = tmp;
		//make sure path doesn't end in '/'
		if(!program_dir.empty() && program_dir[program_dir.size()-1] == '/'){
			program_dir.erase(program_dir.size()-1);
		}
	}
	if(!program_dir.empty()){
		program_dir += "/.p2p/";
	}
	db_file_name = "DB";
	non_set_func_called = false;
}

path::static_wrap::static_objects & path::static_wrap::get()
{
	boost::call_once(once_flag, _get);
	return _get();
}

path::static_wrap::static_objects & path::static_wrap::_get()
{
	static static_objects SO;
	return SO;
}
//END static_wrap

void path::create_dirs()
{
	boost::recursive_mutex::scoped_lock lock(static_wrap::get().mutex);
	static_wrap::get().non_set_func_called = true;
	try{
		#ifndef _WIN32
		//program directory always in current directory on windows
		if(!static_wrap::get().program_dir.empty()){
			boost::filesystem::create_directory(static_wrap::get().program_dir);
		}
		#endif
		boost::filesystem::create_directory(download_dir());
		boost::filesystem::create_directory(load_dir());
		boost::filesystem::create_directory(load_bad_dir());
		boost::filesystem::create_directory(share_dir());
		boost::filesystem::create_directory(tmp_dir());
	}catch(const std::exception & e){
		LOG << e.what();
		exit(1);
	}
}

std::string path::db_file()
{
	boost::recursive_mutex::scoped_lock lock(static_wrap::get().mutex);
	static_wrap::get().non_set_func_called = true;
	return static_wrap::get().program_dir + static_wrap::get().db_file_name;
}

std::string path::download_dir()
{
	boost::recursive_mutex::scoped_lock lock(static_wrap::get().mutex);
	static_wrap::get().non_set_func_called = true;
	return static_wrap::get().program_dir + "download/";
}

std::string path::load_bad_dir()
{
	boost::recursive_mutex::scoped_lock lock(static_wrap::get().mutex);
	static_wrap::get().non_set_func_called = true;
	return static_wrap::get().program_dir + "load/bad/";
}

std::string path::load_dir()
{
	boost::recursive_mutex::scoped_lock lock(static_wrap::get().mutex);
	static_wrap::get().non_set_func_called = true;
	return static_wrap::get().program_dir + "load/";
}

void path::remove_tmp_tree_files()
{
	boost::recursive_mutex::scoped_lock lock(static_wrap::get().mutex);
	static_wrap::get().non_set_func_called = true;
	namespace fs = boost::filesystem;
	boost::uint64_t size;
	fs::path path = fs::system_complete(fs::path(tmp_dir(), fs::native));
	if(fs::exists(path)){
		try{
			for(fs::directory_iterator it_cur(path), it_end; it_cur != it_end;
				++it_cur)
			{
				if(it_cur->path().filename().find("hash_tree_", 0) == 0){
					fs::remove(it_cur->path());
				}
			}
		}catch(const std::exception & e){
			LOG << e.what();
		}
	}
}

void path::set_db_file_name(const std::string & name)
{
	boost::recursive_mutex::scoped_lock lock(static_wrap::get().mutex);
	assert(static_wrap::get().non_set_func_called == false);
	static_wrap::get().db_file_name = name;
}

void path::set_program_dir(const std::string & path)
{
	boost::recursive_mutex::scoped_lock lock(static_wrap::get().mutex);
	assert(static_wrap::get().non_set_func_called == false);
	static_wrap::get().program_dir = path;
}

std::string path::share_dir()
{
	boost::recursive_mutex::scoped_lock lock(static_wrap::get().mutex);
	static_wrap::get().non_set_func_called = true;
	return static_wrap::get().program_dir + "share/";
}

std::string path::tmp_dir()
{
	boost::recursive_mutex::scoped_lock lock(static_wrap::get().mutex);
	static_wrap::get().non_set_func_called = true;
	return static_wrap::get().program_dir + "tmp/";
}

std::string path::tree_file()
{
	boost::recursive_mutex::scoped_lock lock(static_wrap::get().mutex);
	static_wrap::get().non_set_func_called = true;
	std::stringstream ss;
	ss << static_wrap::get().program_dir << "tmp/tree_" << boost::this_thread::get_id();
	return ss.str();
}
