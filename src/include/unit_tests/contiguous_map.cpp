//include
#include <contiguous_map.hpp>
#include <logger.hpp>

bool test_iterator()
{
	contiguous_map<int, char> C(0,5);
	C.insert(std::make_pair(0,'x'));
	C.insert(std::make_pair(1,'x'));
	C.insert(std::make_pair(3,'x'));
	C.insert(std::make_pair(4,'x'));

	contiguous_map<int, char>::iterator iter_cur, iter_end;
	iter_cur = C.begin();
	iter_end = C.end();
	if(iter_cur == iter_end){ return false; }
	if(iter_cur->first != 0){ return false; }
	if((*iter_cur).first != 0){ return false; }
	++iter_cur;
	if(iter_cur->first != 1){ return false; }
	iter_cur++;
	if(iter_cur->first != 3){ return false; }
	++iter_cur;
	if(iter_cur->first != 4){ return false; }
	++iter_cur;
	if(iter_cur != iter_end){ return false; }

	return true;
}

bool test_contiguous_iterator()
{
	contiguous_map<int, char> C(0,5);
	C.insert(std::make_pair(0,'x'));
	C.insert(std::make_pair(1,'x'));
	C.insert(std::make_pair(3,'x'));
	C.insert(std::make_pair(4,'x'));

	contiguous_map<int, char>::contiguous_iterator iter_cur, iter_end;
	iter_cur = C.begin_contiguous();
	iter_end = C.end_contiguous();
	if(iter_cur == iter_end){ return false; }
	if(iter_cur->first != 0){ return false; }
	if((*iter_cur).first != 0){ return false; }
	++iter_cur;
	if(iter_cur->first != 1){ return false; }
	iter_cur++;
	if(iter_cur != iter_end){ return false; }

	return true;
}

bool test_incontiguous_iterator()
{
	contiguous_map<int, char> C(0,5);
	C.insert(std::make_pair(0,'x'));
	C.insert(std::make_pair(1,'x'));
	C.insert(std::make_pair(3,'x'));
	C.insert(std::make_pair(4,'x'));

	contiguous_map<int, char>::incontiguous_iterator iter_cur, iter_end;
	iter_cur = C.begin_incontiguous();
	iter_end = C.end_incontiguous();
	if(iter_cur == iter_end){ return false; }
	if(iter_cur->first != 3){ return false; }
	if((*iter_cur).first != 3){ return false; }
	++iter_cur;
	if(iter_cur->first != 4){ return false; }
	iter_cur++;
	if(iter_cur != iter_end){ return false; }

	return true;
}

bool test_highest_contiguous()
{
	contiguous_map<int, char> C(0,5);
	C.insert(std::make_pair(0,'x'));
	C.insert(std::make_pair(1,'x'));
	C.insert(std::make_pair(3,'x'));
	C.insert(std::make_pair(4,'x'));

	int HC;
	if(C.highest_contiguous(HC)){
		return HC == 1;
	}else{
		return false;
	}
}

bool test_trim_contiguous()
{
	contiguous_map<int, char> C(0,5);
	C.insert(std::make_pair(0,'x'));
	C.insert(std::make_pair(1,'x'));
	C.insert(std::make_pair(3,'x'));
	C.insert(std::make_pair(4,'x'));
	C.trim_contiguous();

	return C.begin()->first == 3;
}

bool test_trim()
{
	contiguous_map<int, char> C(0,5);
	C.insert(std::make_pair(0,'x'));
	C.insert(std::make_pair(1,'x'));
	C.insert(std::make_pair(3,'x'));
	C.insert(std::make_pair(4,'x'));

	C.trim(0);
	if(C.begin()->first != 0){ return false; }
	C.trim(3);
	if(C.begin()->first != 3){ return false; }
	C.trim(5);
	if(C.begin() != C.end()){ return false; }

	return true;
}

bool test_empty()
{
	//if fail these would generally segfault
	contiguous_map<int, char> C(0,5);
	contiguous_map<int, char>::contiguous_iterator c_iter;
	c_iter = C.begin_contiguous();
	c_iter = C.end_contiguous();
	contiguous_map<int, char>::incontiguous_iterator i_iter;
	i_iter = C.begin_incontiguous();
	i_iter = C.end_incontiguous();
	return true;
}

int main()
{
	if(!test_iterator()){
		LOGGER; exit(1);
	}
	if(!test_contiguous_iterator()){
		LOGGER; exit(1);
	}
	if(!test_incontiguous_iterator()){
		LOGGER; exit(1);
	}
	if(!test_highest_contiguous()){
		LOGGER; exit(1);
	}
	if(!test_trim_contiguous()){
		LOGGER; exit(1);
	}
	if(!test_trim()){
		LOGGER; exit(1);
	}

	//make sure nothing blows up when container empty
	if(!test_empty()){
		LOGGER; exit(1);
	}

	return 0;
}
