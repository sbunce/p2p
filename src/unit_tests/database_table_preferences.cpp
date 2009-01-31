//custom
#include "../database.hpp"
#include "../global.hpp"

int main()
{
	database::connection DB;
	DB.query("DROP TABLE IF EXISTS preferences");
	database::table::preferences DBP;
	if(DBP.get_client_connections() != global::CLIENT_CONNECTIONS){
		LOGGER; exit(1);
	}
	if(DBP.get_download_directory() != global::DOWNLOAD_DIRECTORY){
		LOGGER; exit(1);
	}
	if(DBP.get_download_rate() != global::DOWNLOAD_RATE){
		LOGGER; exit(1);
	}
	if(DBP.get_server_connections() != global::SERVER_CONNECTIONS){
		LOGGER; exit(1);
	}
	if(DBP.get_share_directory() != global::SHARE_DIRECTORY){
		LOGGER; exit(1);
	}
	if(DBP.get_upload_rate() != global::UPLOAD_RATE){
		LOGGER; exit(1);
	}
	DBP.set_client_connections(123);
	if(DBP.get_client_connections() != 123){
		LOGGER; exit(1);
	}
	DBP.set_download_directory("ABC");
	if(DBP.get_download_directory() != "ABC"){
		LOGGER; exit(1);
	}
	DBP.set_download_rate(123);
	if(DBP.get_download_rate() != 123){
		LOGGER; exit(1);
	}
	DBP.set_server_connections(123);
	if(DBP.get_server_connections() != 123){
		LOGGER; exit(1);
	}
	DBP.set_share_directory("ABC");
	if(DBP.get_share_directory() != "ABC"){
		LOGGER; exit(1);
	}
	DBP.set_upload_rate(123);
	if(DBP.get_upload_rate() != 123){
		LOGGER; exit(1);
	}

	return 0;
}
