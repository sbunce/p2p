//custom
#include "../DB_client_preferences.h"
#include "../global.h"

//std
#include <iostream>

int main()
{
	//initial tests fail without this
	std::remove(global::DATABASE_PATH.c_str());

	DB_client_preferences DBCP;

	if(DBCP.get_download_directory() != global::DOWNLOAD_DIRECTORY){
		std::cout << "failed initial download directory test\n";
		return 1;
	}

	if(DBCP.get_speed_limit() != global::DOWN_SPEED){
		std::cout << "failed initial download speed test\n";
		return 1;
	}

	if(DBCP.get_max_connections() != global::MAX_CONNECTIONS){
		std::cout << "failed initial max connection test\n";
		return 1;
	}

	DBCP.set_download_directory("abc");
	DBCP.set_speed_limit(123);
	DBCP.set_max_connections(123);

	if(DBCP.get_download_directory() != "abc"){
		std::cout << "failed initial download directory test\n";
		return 1;
	}

	if(DBCP.get_speed_limit() != 123){
		std::cout << "failed initial download speed test\n";
		return 1;
	}

	if(DBCP.get_max_connections() != 123){
		std::cout << "failed initial max connection test\n";
		return 1;
	}

	return 0;
}
