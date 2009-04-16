//custom
#include "../database.hpp"
#include "../settings.hpp"

//include
#include <logger.hpp>

//std
#include <string>

database::connection DB("DB");

class test
{
public:
	test(){}
	int call_back(int columns, char ** response, char ** column_name)
	{
		if(strcmp(response[0], "abc") != 0){
			return 1;
		}
		return 0;
	}

	int call_back_with_object(std::string * str, int columns, char ** response, char ** column_name)
	{
		if(*str != "test"){
			return 1;
		}
		if(strcmp(response[0], "abc") != 0){
			return 1;
		}
		return 0;
	}

	int call_back_with_object_reference(std::string & str, int columns, char ** response, char ** column_name)
	{
		if(str != "test"){
			return 1;
		}
		if(strcmp(response[0], "abc") != 0){
			return 1;
		}
		return 0;
	}
};

int call_back(int columns, char ** response, char ** column_name)
{
	if(strcmp(response[0], "abc") != 0){
		return 1;
	}
	return 0;
}

int call_back_with_object(std::string * str, int columns, char ** response, char ** column_name)
{
	if(*str != "test"){
		return 1;
	}
	if(strcmp(response[0], "abc") != 0){
		return 1;
	}
	return 0;
}

int call_back_with_object_reference(std::string & str, int columns, char ** response, char ** column_name)
{
	if(str != "test"){
		return 1;
	}
	if(strcmp(response[0], "abc") != 0){
		return 1;
	}
	return 0;
}

void test_call_backs()
{
	DB.query("DROP TABLE IF EXISTS sqlite3_wrapper");
	if(DB.query("CREATE TABLE sqlite3_wrapper(test_text TEXT)") != SQLITE_OK){
		LOGGER; exit(1);
	}
	if(DB.query("INSERT INTO sqlite3_wrapper(test_text) VALUES ('abc')") != SQLITE_OK){
		LOGGER; exit(1);
	}
	//function call back
	if(DB.query("SELECT test_text FROM sqlite3_wrapper", call_back) != SQLITE_OK){
		LOGGER; exit(1);
	}
	//function call back with object
	std::string str = "test";
	if(DB.query("SELECT test_text FROM sqlite3_wrapper", call_back_with_object, &str) != SQLITE_OK){
		LOGGER; exit(1);
	}
	//function call back with an object by reference
	if(DB.query("SELECT test_text FROM sqlite3_wrapper", call_back_with_object_reference, str) != SQLITE_OK){
		LOGGER; exit(1);
	}
	//member function call back
	test Test;
	if(DB.query("SELECT test_text FROM sqlite3_wrapper", &Test, &test::call_back) != SQLITE_OK){
		LOGGER; exit(1);
	}
	//member function call back with object
	if(DB.query("SELECT test_text FROM sqlite3_wrapper", &Test, &test::call_back_with_object, &str) != SQLITE_OK){
		LOGGER; exit(1);
	}
	//member function call back with object by reference
	if(DB.query("SELECT test_text FROM sqlite3_wrapper", &Test, &test::call_back_with_object_reference, str) != SQLITE_OK){
		LOGGER; exit(1);
	}
}

int main()
{
	test_call_backs();
	//blob functions are tested in hash_tree unit test
	return 0;
}
