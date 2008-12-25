//custom
#include "../DB_blacklist.h"

//std
#include <iostream>

int main()
{
	std::string IP_1 = "1.1.1.1", IP_2 = "2.2.2.2";

	int state = 0;
	if(DB_blacklist::modified(state)){
		std::cout << "modified test 1 failed\n";
		return 1;
	}

	DB_blacklist::add(IP_1);

	if(!DB_blacklist::modified(state)){
		std::cout << "modified test 2 failed\n";
		return 1;
	}

	if(!DB_blacklist::is_blacklisted(IP_1)){
		std::cout << "is_blacklisted test 1 failed\n";
		return 1;
	}

	if(DB_blacklist::is_blacklisted(IP_2)){
		std::cout << "is_blacklisted test 2 failed\n";
		return 1;
	}

	return 0;
}
