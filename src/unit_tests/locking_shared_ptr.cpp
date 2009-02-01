/*
This unit test does not test the thread safety of the locking_shared_ptr. It
only makes sure everything compiles ok. The locking_shared_ptr code is simple
but subtle. You have to look at the code and just convince yourself it works.
*/

//custom
#include "../global.hpp"
#include "../locking_shared_ptr.hpp"

//std
#include <vector>

int main()
{
	//call ctor that does instantiation
	locking_shared_ptr<int> x;
	**x = 0;
	if(**x != 0){
		LOGGER; exit(1);
	}

	//call ctor that takes pointer as parameter
	locking_shared_ptr<int> y(new int(0));
	if(**y != 0){
		LOGGER; exit(1);
	}

	//test inline deref
	locking_shared_ptr<std::vector<int> > vec;
	vec->empty();

	return 0;
}
