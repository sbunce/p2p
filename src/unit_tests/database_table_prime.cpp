//custom
#include "../database.hpp"
#include "../global.hpp"

int main()
{
	database::connection DB;
	DB.query("DROP TABLE IF EXISTS prime");
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
