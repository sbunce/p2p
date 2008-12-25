//custom
#include "../DB_server_preferences.h"
#include "../global.h"

//std
#include <iostream>

int main()
{
	//initial tests fail without this
	std::remove(global::DATABASE_PATH.c_str());

	DB_server_preferences DBSP;

	if(DBSP.get_share_directory() != global::SHARE_DIRECTORY){
		std::cout << "failed initial share directory test\n";
		return 1;
	}

	if(DBSP.get_speed_limit() != global::UP_SPEED){
		std::cout << "failed initial speed test\n";
		return 1;
	}

	if(DBSP.get_max_connections() != global::MAX_CONNECTIONS){
		std::cout << "failed initial max connection test\n";
		return 1;
	}

	DBSP.set_share_directory("ABC");
	DBSP.set_speed_limit(123);
	DBSP.set_max_connections(123);

	if(DBSP.get_share_directory() != "ABC"){
		std::cout << "failed share directory test\n";
		return 1;
	}

	if(DBSP.get_speed_limit() != 123){
		std::cout << "failed speed test\n";
		return 1;
	}

	if(DBSP.get_max_connections() != 123){
		std::cout << "failed max connection test\n";
		return 1;
	}

	return 0;
}
