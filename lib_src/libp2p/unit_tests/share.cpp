//custom
#include "../share.hpp"

//include
#include <logger.hpp>

int main()
{
	share Share;

	//test files
	file_info FI_1("ABC", "/foo", 123);
	file_info FI_2("DEF", "/foo/bar", 123);

	Share.insert_update(FI_1);
	Share.insert_update(FI_2);

	share::const_file_iterator iter;

	//lookup file_1 by hash
	iter = Share.lookup_hash(FI_1.hash);
	if(iter != Share.end_file()){
		if(iter->path != FI_1.path){
			LOGGER; exit(1);
		}
	}else{
		LOGGER; exit(1);
	}

	//lookup file_1 by path
	iter = Share.lookup_path(FI_1.path);
	if(iter != Share.end_file()){
		if(iter->path != FI_1.path){
			LOGGER; exit(1);
		}
	}else{
		LOGGER; exit(1);
	}

	//lookup file_2 by hash
	iter = Share.lookup_hash(FI_2.hash);
	if(iter != Share.end_file()){
		if(iter->path != FI_2.path){
			LOGGER; exit(1);
		}
	}else{
		LOGGER; exit(1);
	}

	//lookup file_2 by path
	iter = Share.lookup_path(FI_2.path);
	if(iter != Share.end_file()){
		if(iter->path != FI_2.path){
			LOGGER; exit(1);
		}
	}else{
		LOGGER; exit(1);
	}

	//test const_iterator
	share::const_file_iterator iter_cur = Share.begin_file(),
		iter_end = Share.end_file();
	if(iter_cur->path != FI_1.path){
		LOGGER; exit(1);
	}
	++iter_cur;
	if(iter_cur->path != FI_2.path){
		LOGGER; exit(1);
	}
}
