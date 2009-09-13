//custom
#include "../shared_files.hpp"

//include
#include <logger.hpp>

/*
void erase(const std::string & path);
void insert_update(const file & File);
boost::shared_ptr<file> lookup_hash(const std::string & hash);
boost::shared_ptr<file> lookup_path(const std::string & path);
*/

int main()
{
	shared_files SF;

	//test files
	shared_files::file file_1("ABC", "/foo", 123);
	shared_files::file file_2("DEF", "/foo/bar", 123);

	SF.insert_update(file_1);
	SF.insert_update(file_2);

	//lookup file_1 by hash
	if(boost::shared_ptr<const shared_files::file> F = SF.lookup_hash(file_1.hash)){
		if(*F != file_1){
			LOGGER; exit(1);
		}
	}else{
		LOGGER; exit(1);
	}

	//lookup file_1 by path
	if(boost::shared_ptr<const shared_files::file> F = SF.lookup_path(file_1.path)){
		if(*F != file_1){
			LOGGER; exit(1);
		}
	}else{
		LOGGER; exit(1);
	}

	//lookup file_2 by hash
	if(boost::shared_ptr<const shared_files::file> F = SF.lookup_hash(file_2.hash)){
		if(*F != file_2){
			LOGGER; exit(1);
		}
	}else{
		LOGGER; exit(1);
	}

	//lookup file_2 by path
	if(boost::shared_ptr<const shared_files::file> F = SF.lookup_path(file_2.path)){
		if(*F != file_2){
			LOGGER; exit(1);
		}
	}else{
		LOGGER; exit(1);
	}

	//test const_iterator
	shared_files::const_iterator iter_cur = SF.begin(), iter_end = SF.end();
	if(*iter_cur != file_1){
		LOGGER; exit(1);
	}
	++iter_cur;
	if(*iter_cur != file_2){
		LOGGER; exit(1);
	}
}
