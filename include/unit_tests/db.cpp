//include
#include <db.hpp>
#include <logger.hpp>

//standard
#include <cstring>
#include <string>

int fail(0);

int call_back(int columns, char ** response, char ** column_name)
{
	if(std::strcmp(response[0], "abc") != 0){
		return 1;
	}
	return 0;
}

int main()
{
	database::connection DB("database_connection.db");
	DB.query("DROP TABLE IF EXISTS sqlite3_wrapper");
	DB.query("CREATE TABLE sqlite3_wrapper(test_text TEXT)");
	DB.query("INSERT INTO sqlite3_wrapper(test_text) VALUES ('abc')");
	DB.query("SELECT test_text FROM sqlite3_wrapper", &call_back);
	return fail;
}
