//custom
#include "../database.hpp"

//include
#include <iostream>

int main()
{
	database::pool::proxy DB;
	DB->query("SELECT * from sqlite_master");
	database::pool::get_proxy()->query("SELECT * from sqlite_master");
	database::pool::singleton().clear();
}
