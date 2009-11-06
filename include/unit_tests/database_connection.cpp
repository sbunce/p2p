//include
#include <database_connection.hpp>
#include <logger.hpp>

//standard
#include <cstring>
#include <string>

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
	if(DB.query("CREATE TABLE sqlite3_wrapper(test_text TEXT)") != SQLITE_OK){
		LOGGER; exit(1);
	}
	if(DB.query("INSERT INTO sqlite3_wrapper(test_text) VALUES ('abc')") != SQLITE_OK){
		LOGGER; exit(1);
	}
	//function call back
	if(DB.query("SELECT test_text FROM sqlite3_wrapper", &call_back) != SQLITE_OK){
		LOGGER; exit(1);
	}
}
