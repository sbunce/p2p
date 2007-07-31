/*
The functions and class in the sqlite namespace are a hack to get around the
problem of not being able to pass sqlite member functions as callback functions.

This is not ideal and if this is used for anything demanding there could be
performance problems.
*/

#ifndef H_SQLITE
#define H_SQLITE

#include <iostream>
#include <string>
#include <vector>

#include <sqlite3.h>

class test
{
public:

	int sqlite_callBack(void * nothing, int & columnsRetrieved, char ** queryResponse, char ** columnName)
	{
		for(int x=0; x<columnsRetrieved; x++){

			int y = 0;
			while(queryResponse[x][y]){
				std::cout << queryResponse[x][y++];
			}

			std::cout << "\n";
		}

		return 0;
	}

	static int wrapper_sqlite_callBack(void * nothing, int columnsRetrieved, char ** queryResponse, char ** columnName)
	{
		test * mySelf = (test *)nothing;
		mySelf->sqlite_callBack(nothing, columnsRetrieved, queryResponse, columnName);
	}

};

int main(int argc, char argv[])
{
	test Test;

	sqlite3 * sqlite3;
	int returnCode = sqlite3_open("p2p", &sqlite3);

	//create table
	sqlite3_exec(sqlite3, "CREATE TABLE IF NOT EXISTS test (col1 TEXT, col2 NUMERIC, col3 INTEGER, col4 REAL, col5 NONE)", NULL, NULL, NULL);

	//inserts
	sqlite3_exec(sqlite3, "INSERT INTO test VALUES('HELLO WORLD', '69.6969', '69', 'WTF IS REAL', 'OH BLARG NO DATATYPE')", NULL, NULL, NULL);
	sqlite3_exec(sqlite3, "INSERT INTO test VALUES('test1', '2', '3', '4', '5')", NULL, NULL, NULL);
	sqlite3_exec(sqlite3, "INSERT INTO test VALUES('test2', '2', '3', '4', '5')", NULL, NULL, NULL);

	//selects
	sqlite3_exec(sqlite3, "SELECT * FROM test", test::wrapper_sqlite_callBack, (void *) &Test, NULL);

	//drop
	sqlite3_exec(sqlite3, "DROP TABLE test;", NULL, NULL, NULL);

	sqlite3_close(sqlite3);
	return 0;
}
#endif

