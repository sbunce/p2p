//custom
#include "../database.hpp"

int main()
{
	database::table::preferences::set_max_download_rate(123);
	if(database::table::preferences::get_max_download_rate() != 123){
		LOGGER; exit(1);
	}
	database::table::preferences::set_max_connections(123);
	if(database::table::preferences::get_max_connections() != 123){
		LOGGER; exit(1);
	}
	database::table::preferences::set_max_upload_rate(123);
	if(database::table::preferences::get_max_upload_rate() != 123){
		LOGGER; exit(1);
	}
}
