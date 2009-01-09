//custom
#include "../contiguous_set.h"

//std
#include <iostream>

bool test_iterator()
{
	contiguous_set<int> C(0,5);
	C.insert(0);
	C.insert(1);
	C.insert(3);
	C.insert(4);

	contiguous_set<int>::iterator iter_cur, iter_end;
	iter_cur = C.begin();
	iter_end = C.end();
	if(iter_cur == iter_end){ return false; }
	if(*iter_cur != 0){ return false; }
	++iter_cur;
	if(*iter_cur != 1){ return false; }
	iter_cur++;
	if(*iter_cur != 3){ return false; }
	++iter_cur;
	if(*iter_cur != 4){ return false; }
	++iter_cur;
	if(iter_cur != iter_end){ return false; }

	return true;
}

bool test_contiguous_iterator()
{
	contiguous_set<int> C(0,5);
	C.insert(0);
	C.insert(1);
	C.insert(3);
	C.insert(4);

	contiguous_set<int>::contiguous_iterator iter_cur, iter_end;
	iter_cur = C.begin_contiguous();
	iter_end = C.end_contiguous();
	if(iter_cur == iter_end){ return false; }
	if(*iter_cur != 0){ return false; }
	++iter_cur;
	if(*iter_cur != 1){ return false; }
	iter_cur++;
	if(iter_cur != iter_end){ return false; }

	return true;
}

bool test_incontiguous_iterator()
{
	contiguous_set<int> C(0,5);
	C.insert(0);
	C.insert(1);
	C.insert(3);
	C.insert(4);

	contiguous_set<int>::incontiguous_iterator iter_cur, iter_end;
	iter_cur = C.begin_incontiguous();
	iter_end = C.end_incontiguous();
	if(iter_cur == iter_end){ return false; }
	if(*iter_cur != 3){ return false; }
	++iter_cur;
	if(*iter_cur != 4){ return false; }
	iter_cur++;
	if(iter_cur != iter_end){ return false; }

	return true;
}

bool test_highest_contiguous()
{
	contiguous_set<int> C(0,5);
	C.insert(0);
	C.insert(1);
	C.insert(3);
	C.insert(4);

	int HC;
	if(C.highest_contiguous(HC)){
		return HC == 1;
	}else{
		return false;
	}
}

bool test_trim_contiguous()
{
	contiguous_set<int> C(0,5);
	C.insert(0);
	C.insert(1);
	C.insert(3);
	C.insert(4);
	C.trim_contiguous();

	return *C.begin() == 3;
}

bool test_trim()
{
	contiguous_set<int> C(0,5);
	C.insert(0);
	C.insert(1);
	C.insert(3);
	C.insert(4);

	C.trim(0);
	if(*C.begin() != 0){ return false; }
	C.trim(3);
	if(*C.begin() != 3){ return false; }
	C.trim(5);
	if(C.begin() != C.end()){ return false; }

	return true;
}

bool test_empty()
{
	//if fail these would generally segfault
	contiguous_set<int> C(0,5);
	contiguous_set<int>::contiguous_iterator c_iter;
	c_iter = C.begin_contiguous();
	c_iter = C.end_contiguous();
	contiguous_set<int>::incontiguous_iterator i_iter;
	i_iter = C.begin_incontiguous();
	i_iter = C.end_incontiguous();
	return true;
}

int main()
{
	if(!test_iterator()){
		std::cout << "failed iterator test\n";
		return 1;
	}

	if(!test_contiguous_iterator()){
		std::cout << "failed contiguous iterator test\n";
		return 1;
	}

	if(!test_incontiguous_iterator()){
		std::cout << "failed incontiguous iterator test\n";
		return 1;
	}

	if(!test_highest_contiguous()){
		std::cout << "failed highest contiguous test\n";
		return 1;
	}

	if(!test_trim_contiguous()){
		std::cout << "failed trim contiguous test\n";
		return 1;
	}

	if(!test_trim()){
		std::cout << "failed trim test\n";
		return 1;
	}

	//make sure nothing blows up when container empty
	if(!test_empty()){
		std::cout << "failed empty test\n";
		return 1;
	}

	return 0;
}
