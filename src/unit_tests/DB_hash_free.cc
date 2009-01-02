//custom
#include "../DB_prime.h"
#include "../global.h"

//std
#include <iostream>

int main()
{
	//initial tests fail without this
	std::remove(global::DATABASE_PATH.c_str());

/*
	if(){
		std::cout << "failed store/retrieve test\n";
		return 1;
	}
*/
	return 0;
}
