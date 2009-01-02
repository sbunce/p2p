//custom
#include "../global.h"
#include "../sqlite3_wrapper.h"

//std
#include <iostream>
#include <string>

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

int main()
{
	//for debug purposes
	std::remove(global::DATABASE_PATH.c_str());

	//create database
	sqlite3_wrapper DB(global::DATABASE_PATH);
	if(DB.query("CREATE TABLE IF NOT EXISTS test (bla TEXT)") != 0){
		std::cout << "failed query 1\n";
		return 1;
	}
	if(DB.query("INSERT INTO test VALUES ('abc')") != 0){
		std::cout << "failed query 2\n";
		return 1;
	}

	//test function call back
	if(DB.query("SELECT bla FROM test", call_back) != 0){
		std::cout << "failed query 3\n";
		return 1;
	}

	//test function call back with object
	std::string str = "test";
	if(DB.query("SELECT bla FROM test", call_back_with_object, &str) != 0){
		std::cout << "failed query 4\n";
		return 1;
	}

	//test function call back with an object by reference
	if(DB.query("SELECT bla FROM test", call_back_with_object_reference, str) != 0){
		std::cout << "failed query 5\n";
		return 1;
	}

	//test member function call back
	test Test;
	if(DB.query("SELECT bla FROM test", &Test, &test::call_back) != 0){
		std::cout << "failed query 6\n";
		return 1;
	}

	//test member function call back with object
	if(DB.query("SELECT bla FROM test", &Test, &test::call_back_with_object, &str) != 0){
		std::cout << "failed query 7\n";
		return 1;
	}

	//test member function call back with object by reference
	if(DB.query("SELECT bla FROM test", &Test, &test::call_back_with_object_reference, str) != 0){
		std::cout << "failed query 8\n";
		return 1;
	}

	return 0;
}
