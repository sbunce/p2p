//custom
#include "../database.hpp"
#include "../settings.hpp"

int main()
{
	database::connection DB("DB");
	database::init::run("DB");

	database::table::preferences::set_max_download_rate(123, DB);
	if(database::table::preferences::get_max_download_rate(DB) != 123){
		LOGGER; exit(1);
	}
	database::table::preferences::set_max_connections(123, DB);
	if(database::table::preferences::get_max_connections(DB) != 123){
		LOGGER; exit(1);
	}
	database::table::preferences::set_max_upload_rate(123, DB);
	if(database::table::preferences::get_max_upload_rate(DB) != 123){
		LOGGER; exit(1);
	}
}
