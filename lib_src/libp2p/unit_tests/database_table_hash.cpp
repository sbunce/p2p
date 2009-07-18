//custom
#include "../database.hpp"

//standard
#include <cstring>

int main()
{
	database::table::hash::clear();
	int test_tree_size = 123;
	if(database::table::hash::exists("ABC", test_tree_size)){
		LOGGER; exit(1);
	}
	database::table::hash::tree_allocate("ABC", test_tree_size);
	if(!database::table::hash::exists("ABC", test_tree_size)){
		LOGGER; exit(1);
	}
	database::table::hash::state State;
	database::table::hash::get_state("ABC", test_tree_size, State);
	if(State != database::table::hash::RESERVED){
		LOGGER; exit(1);
	}
	database::table::hash::set_state("ABC", test_tree_size, database::table::hash::COMPLETE);
	State = database::table::hash::DOWNLOADING;
	database::table::hash::get_state("ABC", test_tree_size, State);
	if(State != database::table::hash::COMPLETE){
		LOGGER; exit(1);
	}
	database::blob Blob = database::table::hash::tree_open("ABC", test_tree_size);
	char * input_buff = static_cast<char *>(std::malloc(test_tree_size));
	std::memset(input_buff, 255, test_tree_size);
	if(!database::pool::get_proxy()->blob_write(Blob, input_buff, test_tree_size, 0)){
		LOGGER; exit(1);
	}
	char * output_buff = static_cast<char *>(std::malloc(test_tree_size));
	std::memset(output_buff, 0, test_tree_size);
	if(!database::pool::get_proxy()->blob_read(Blob, output_buff, test_tree_size, 0)){
		LOGGER; exit(1);
	}
	if(std::memcmp(input_buff, output_buff, test_tree_size) != 0){
		LOGGER; exit(1);
	}
	database::table::hash::delete_tree("ABC", test_tree_size);
	if(database::table::hash::exists("ABC", test_tree_size)){
		LOGGER; exit(1);
	}
}
