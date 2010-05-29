//include
#include <db.hpp>
#include <logger.hpp>
#include <unit_test.hpp>

//standard
#include <cstring>
#include <string>

int fail(0);

int call_back(int columns, char ** response, char ** column_name)
{
	if(std::strcmp(response[0], "abc") != 0){
		LOG; ++fail;
	}
	return 0;
}

int main()
{
	unit_test::timeout();
	db::connection DB("db.db");
	DB.query("DROP TABLE IF EXISTS sqlite3_wrapper");
	DB.query("CREATE TABLE sqlite3_wrapper(test TEXT)");
	DB.query("INSERT INTO sqlite3_wrapper VALUES ('abc')");
	DB.query("SELECT test FROM sqlite3_wrapper", &call_back);
	return fail;
}
