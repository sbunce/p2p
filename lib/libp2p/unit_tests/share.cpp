//custom
#include "../share.hpp"

//include
#include <logger.hpp>
#include <unit_test.hpp>

int fail(0);

int main()
{
	unit_test::timeout();

	//test files
	file_info FI_1("ABC", "/foo", 123, 123);
	file_info FI_2("DEF", "/foo/bar", 123, 123);

	share::singleton()->insert(FI_1);
	share::singleton()->insert(FI_2);

	share::const_file_iterator iter;

	//lookup file_1 by hash
	iter = share::singleton()->find_hash(FI_1.hash);
	if(iter != share::singleton()->end_file()){
		if(iter->path != FI_1.path){
			LOG; ++fail;
		}
	}else{
		LOG; ++fail;
	}

	//lookup file_1 by path
	iter = share::singleton()->find_path(FI_1.path);
	if(iter != share::singleton()->end_file()){
		if(iter->path != FI_1.path){
			LOG; ++fail;
		}
	}else{
		LOG; ++fail;
	}

	//lookup file_2 by hash
	iter = share::singleton()->find_hash(FI_2.hash);
	if(iter != share::singleton()->end_file()){
		if(iter->path != FI_2.path){
			LOG; ++fail;
		}
	}else{
		LOG; ++fail;
	}

	//lookup file_2 by path
	iter = share::singleton()->find_path(FI_2.path);
	if(iter != share::singleton()->end_file()){
		if(iter->path != FI_2.path){
			LOG; ++fail;
		}
	}else{
		LOG; ++fail;
	}

	//test const_iterator
	share::const_file_iterator it_cur = share::singleton()->begin_file(),
		it_end = share::singleton()->end_file();
	if(it_cur->path != FI_1.path){
		LOG; ++fail;
	}
	++it_cur;
	if(it_cur->path != FI_2.path){
		LOG; ++fail;
	}
	return fail;
}
