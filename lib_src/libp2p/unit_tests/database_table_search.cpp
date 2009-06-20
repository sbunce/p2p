//custom
#include "../database.hpp"
#include "../settings.hpp"

int main()
{
	database::connection DB("DB");
	database::init::run("DB");
	DB.query("DROP TABLE IF EXISTS search");

	/*
	There is currently no function to insert in to database. This makes unit test
	impossible. However, an insert function will eventually be needed.
	*/
}
