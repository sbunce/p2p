//custom
#include "../DB_preferences.h"
#include "../global.h"

//std
#include <iostream>

int main()
{
	sqlite3_wrapper DB(global::DATABASE_PATH);
	DB.query("DROP TABLE IF EXISTS preferences");

	DB_preferences DBP;

	if(DBP.get_client_connections() != global::CLIENT_CONNECTIONS){
		std::cout << "failed get_client_connections test\n";
		return 1;
	}

	if(DBP.get_download_directory() != global::DOWNLOAD_DIRECTORY){
		std::cout << "failed get_download_directory test\n";
		return 1;
	}

	if(DBP.get_download_rate() != global::DOWNLOAD_RATE){
		std::cout << "failed get_download_rate test\n";
		return 1;
	}

	if(DBP.get_server_connections() != global::SERVER_CONNECTIONS){
		std::cout << "failed get_server_connections test\n";
		return 1;
	}

	if(DBP.get_share_directory() != global::SHARE_DIRECTORY){
		std::cout << "failed get_share_directory test\n";
		return 1;
	}

	if(DBP.get_upload_rate() != global::UPLOAD_RATE){
		std::cout << "failed get_upload_rate test\n";
		return 1;
	}

	DBP.set_client_connections(123);
	if(DBP.get_client_connections() != 123){
		std::cout << "failed set_client_connections test\n";
		return 1;
	}

	DBP.set_download_directory("abc");
	if(DBP.get_download_directory() != "abc"){
		std::cout << "failed set_download_directory test\n";
		return 1;
	}

	DBP.set_download_rate(123);
	if(DBP.get_download_rate() != 123){
		std::cout << "failed set_download_rate test\n";
		return 1;
	}

	DBP.set_server_connections(123);
	if(DBP.get_server_connections() != 123){
		std::cout << "failed set_server_connections test\n";
		return 1;
	}

	DBP.set_share_directory("abc");
	if(DBP.get_share_directory() != "abc"){
		std::cout << "failed set_share_directory test\n";
		return 1;
	}

	DBP.set_upload_rate(123);
	if(DBP.get_upload_rate() != 123){
		std::cout << "failed set_upload_rate test\n";
		return 1;
	}

	return 0;
}
