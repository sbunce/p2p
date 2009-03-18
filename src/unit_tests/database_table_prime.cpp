//custom
#include "../database.hpp"
#include "../settings.hpp"

int main()
{
	database::init::run();
	database::connection DB;
	DB.query("DELETE FROM prime");
	database::table::prime DBP;

	mpint input("123"); //not prime, only used to test store and fetch
	DBP.add(input);
	mpint output;
	DBP.retrieve(output);

	if(input != output){
		LOGGER; exit(1);
	}

	return 0;
}
