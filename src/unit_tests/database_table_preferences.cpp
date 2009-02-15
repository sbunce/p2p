//custom
#include "../database.hpp"
#include "../global.hpp"

int main()
{
	database::connection DB;
	database::table::preferences DBP;

	DBP.set_download_directory("ABC");
	if(DBP.get_download_directory() != "ABC"){
		LOGGER; exit(1);
	}
	DBP.set_max_download_rate(123);
	if(DBP.get_max_download_rate() != 123){
		LOGGER; exit(1);
	}
	DBP.set_max_connections(123);
	if(DBP.get_max_connections() != 123){
		LOGGER; exit(1);
	}
	DBP.set_share_directory("ABC");
	if(DBP.get_share_directory() != "ABC"){
		LOGGER; exit(1);
	}
	DBP.set_max_upload_rate(123);
	if(DBP.get_max_upload_rate() != 123){
		LOGGER; exit(1);
	}

	return 0;
}
