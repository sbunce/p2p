//custom
#include "../DB_blacklist.h"

//std
#include <iostream>

int main()
{
	sqlite3_wrapper DB(global::DATABASE_PATH);
	DB.query("DROP TABLE IF EXISTS blacklist");

	DB_blacklist DB_Blacklist;
	std::string IP_1 = "1.1.1.1", IP_2 = "2.2.2.2";

	int state = 0;
	if(DB_Blacklist.modified(state)){
		std::cout << "modified test 1 failed\n";
		return 1;
	}

	DB_Blacklist.add(IP_1);

	if(!DB_Blacklist.modified(state)){
		std::cout << "modified test 2 failed\n";
		return 1;
	}

	if(!DB_Blacklist.is_blacklisted(IP_1)){
		std::cout << "is_blacklisted test 1 failed\n";
		return 1;
	}

	if(DB_Blacklist.is_blacklisted(IP_2)){
		std::cout << "is_blacklisted test 2 failed\n";
		return 1;
	}

	return 0;
}
