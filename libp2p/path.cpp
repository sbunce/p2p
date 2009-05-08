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
	if(home().empty()){
		return ".p2p/";
	}else{
		return home() + "/.p2p/";
	}
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

std::string path::share()
{
	if(home().empty()){
		return "share/";
	}else{
		return home() + "/.p2p/share/";
	}
}
