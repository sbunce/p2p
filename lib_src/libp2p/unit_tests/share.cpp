//custom
#include "../share.hpp"

//include
#include <logger.hpp>

int fail(0);

int main()
{
	//test files
	file_info FI_1("ABC", "/foo", 123, 123);
	file_info FI_2("DEF", "/foo/bar", 123, 123);

	share::singleton().insert(FI_1);
	share::singleton().insert(FI_2);

	share::const_file_iterator iter;

	//lookup file_1 by hash
	iter = share::singleton().find_hash(FI_1.hash);
	if(iter != share::singleton().end_file()){
		if(iter->path != FI_1.path){
			LOGGER(logger::utest); ++fail;
		}
	}else{
		LOGGER(logger::utest); ++fail;
	}

	//lookup file_1 by path
	iter = share::singleton().find_path(FI_1.path);
	if(iter != share::singleton().end_file()){
		if(iter->path != FI_1.path){
			LOGGER(logger::utest); ++fail;
		}
	}else{
		LOGGER(logger::utest); ++fail;
	}

	//lookup file_2 by hash
	iter = share::singleton().find_hash(FI_2.hash);
	if(iter != share::singleton().end_file()){
		if(iter->path != FI_2.path){
			LOGGER(logger::utest); ++fail;
		}
	}else{
		LOGGER(logger::utest); ++fail;
	}

	//lookup file_2 by path
	iter = share::singleton().find_path(FI_2.path);
	if(iter != share::singleton().end_file()){
		if(iter->path != FI_2.path){
			LOGGER(logger::utest); ++fail;
		}
	}else{
		LOGGER(logger::utest); ++fail;
	}

	//test const_iterator
	share::const_file_iterator iter_cur = share::singleton().begin_file(),
		iter_end = share::singleton().end_file();
	if(iter_cur->path != FI_1.path){
		LOGGER(logger::utest); ++fail;
	}
	++iter_cur;
	if(iter_cur->path != FI_2.path){
		LOGGER(logger::utest); ++fail;
	}
	return fail;
}
