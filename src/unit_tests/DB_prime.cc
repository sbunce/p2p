//custom
#include "../DB_prime.h"
#include "../global.h"

int main()
{
	sqlite3_wrapper::database DB;
	DB.query("DROP TABLE IF EXISTS prime");
	DB_prime DBP;

	mpint input("123"); //not prime, only used to test store and fetch
	DBP.add(input);
	mpint output;
	DBP.retrieve(output);

	if(input != output){
		LOGGER; exit(1);
	}

	return 0;
}
