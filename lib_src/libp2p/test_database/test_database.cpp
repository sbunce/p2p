/*
This program erases the database used by the p2p program and inserts test data.
This is used to quickly set up testing parameters.
*/

//custom
#include "../database.hpp"

int main()
{
	database::init::drop_all();
	database::init::create_all();
}
