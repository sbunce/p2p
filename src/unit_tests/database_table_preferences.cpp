//custom
#include "../database.hpp"
#include "../global.hpp"

int main()
{
	database::connection DB;
	database::table::preferences DBP;

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
