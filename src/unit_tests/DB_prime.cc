//custom
#include "../DB_prime.h"
#include "../global.h"

//std
#include <iostream>

int main()
{
	sqlite3_wrapper DB(global::DATABASE_PATH);
	DB.query("DROP TABLE IF EXISTS prime");

	DB_prime DBP;

	mpint input("123"); //not prime, only used to test store and fetch
	DBP.add(input);
	mpint output;
	DBP.retrieve(output);

	if(input != output){
		std::cout << "failed store/retrieve test\n";
		return 1;
	}

	return 0;
}
