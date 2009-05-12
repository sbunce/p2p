#include "path.hpp"

//BEGIN static
boost::once_flag path::once_flag = BOOST_ONCE_INIT;
std::string * path::_home;

void path::init()
{
	_home = new std::string();
	const char * tmp = std::getenv("HOME");
	if(tmp != NULL){
		*_home = tmp;
		//make sure path doesn't end in '/'
		if(!_home->empty() && (*_home)[_home->size()-1] == '/'){
			_home->erase(_home->size()-1);
		}
	}
}

std::string & path::home()
{
	boost::call_once(init, once_flag);
	return *_home;
}
//END static

path::path()
{

}

std::string path::database()
{
	if(home().empty()){
		return "DB";
	}else{
		return home() + "/.p2p/DB";
	}
}

std::string path::main()
{
	#ifdef WIN32
	return "";
	#else
	if(home().empty()){
		return ".p2p/";
	}else{
		return home() + "/.p2p/";
	}
	#endif
}

std::string path::download()
{
	if(home().empty()){
		return "download/";
	}else{
		return home() + "/.p2p/download/";
	}
}

std::string path::download_unfinished()
{
	if(home().empty()){
		return "download_unfinished/";
	}else{
		return home() + "/.p2p/download_unfinished/";
	}
}

std::string path::rightside_up()
{
	if(home().empty()){
		std::stringstream ss;
		ss << home() << "temp/rightside_up_" << boost::this_thread::get_id();
		return ss.str();
	}else{
		std::stringstream ss;
		ss << home() << "/.p2p/temp/rightside_up_" << boost::this_thread::get_id();
		return ss.str();
	}
}

std::string path::share()
{
	if(home().empty()){
		return "share/";
	}else{
		return home() + "/.p2p/share/";
	}
}

std::string path::temp()
{
	if(home().empty()){
		return "temp/";
	}else{
		return home() + "/.p2p/temp";
	}
}

std::string path::upside_down()
{
	if(home().empty()){
		std::stringstream ss;
		ss << home() << "temp/upside_down_" << boost::this_thread::get_id();
		return ss.str();
	}else{
		std::stringstream ss;
		ss << home() << "/.p2p/temp/upside_down_" << boost::this_thread::get_id();
		return ss.str();
	}
}
