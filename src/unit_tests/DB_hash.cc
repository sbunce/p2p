//custom
#include "../DB_hash.h"
#include "../global.h"

//std
#include <fstream>
#include <iostream>

//debug call backs for print_table()
int column_name_call_back(int columns_retrieved, char ** response, char ** column_name)
{
	std::cout << "|";
	for(int x=0; x<columns_retrieved; ++x){
		if(column_name[x]){
			std::cout << column_name[x] << "|";
		}
	}
	std::cout << "\n";
	return 0;
}
int data_call_back(int columns_retrieved, char ** response, char ** column_name)
{
	std::cout << "|";
	for(int x=0; x<columns_retrieved; ++x){
		if(response[x]){
			std::cout << response[x] << "|";
		}
	}
	std::cout << "\n";
	return 0;
}

//debug function to print contents of a table
void print_table(const std::string & table_name)
{
	sqlite3_wrapper DB(global::DATABASE_PATH);
	std::stringstream ss;
	ss << "SELECT * FROM " << table_name << " LIMIT 1";
	DB.query(ss.str(), &column_name_call_back);
	ss.str(""); ss.clear();
	ss << "SELECT * FROM " << table_name;
	DB.query(ss.str(), &data_call_back);
}

void random_reserve_use_free(DB_hash & DBH)
{
	std::srand(time(NULL));
	boost::uint64_t offset;
	for(int x=0; x<128; ++x){
		offset = DBH.reserve(std::rand() % 256 + 1); //+1 because zero not allowed
		DBH.use(offset, "123");
	}
}

int check_contiguous_call_back(int columns_retrieved, char ** response,
	char ** column_name)
{
	assert(response[0] && response[1]);
	static boost::uint64_t next_expected = 0;
	boost::uint64_t offset, size;
	std::stringstream ss;
	ss << response[0];
	ss >> offset;
	ss.str(""); ss.clear();
	ss << response[1];
	ss >> size;
	ss.str(""); ss.clear();
	if(offset != next_expected){
		exit(1);
	}else{
		next_expected = offset + size;
		return 0;
	}
}

void check_contiguous()
{
	sqlite3_wrapper DB(global::DATABASE_PATH);
	DB.query("SELECT offset, size FROM hash ORDER BY offset ASC", &check_contiguous_call_back);
}

int main()
{
	sqlite3_wrapper DB(global::DATABASE_PATH);
	DB.query("DROP TABLE IF EXISTS hash");

	DB_hash DBH;
	random_reserve_use_free(DBH);
	check_contiguous();
	//print_table("hash");
	return 0;
}
