//custom
#include "../database.hpp"

//standard
#include <cstring>

int main()
{
	//setup database and make sure hash table clear
	path::unit_test_override("database_table_hash.db");
	database::init::hash();
	database::table::hash::clear();

	//size of our test tree
	int test_tree_size = 123;

	//tree has not been added yet, it should not exist
	if(database::table::hash::exists("ABC", test_tree_size)){
		LOGGER; exit(1);
	}

	//allocating a tree adds it to the table
	database::table::hash::tree_allocate("ABC", test_tree_size);

	//we just added the tree so it should exist
	if(!database::table::hash::exists("ABC", test_tree_size)){
		LOGGER; exit(1);
	}

	/*
	The default state of a hash tree is RESERVED which means the tree is not
	referenced from any other table but there is a row for it, and the blob for
	the tree is allocated. If the state is not changed from RESERVED before the
	program is shut down the row will be deleted when the program is next
	started.
	*/
	database::table::hash::state State;
	database::table::hash::get_state("ABC", test_tree_size, State);
	if(State != database::table::hash::RESERVED){
		LOGGER; exit(1);
	}

	//test changing the state to complete
	database::table::hash::set_state("ABC", test_tree_size, database::table::hash::COMPLETE);
	State = database::table::hash::DOWNLOADING;
	database::table::hash::get_state("ABC", test_tree_size, State);
	if(State != database::table::hash::COMPLETE){
		LOGGER; exit(1);
	}

	//turn on every bit in the tree to test
	database::blob Blob = database::table::hash::tree_open("ABC", test_tree_size);
	char * input_buff = static_cast<char *>(std::malloc(test_tree_size));
	std::memset(input_buff, 255, test_tree_size);
	if(!database::pool::get_proxy()->blob_write(Blob, input_buff, test_tree_size, 0)){
		LOGGER; exit(1);
	}

	//read the tree
	char * output_buff = static_cast<char *>(std::malloc(test_tree_size));
	std::memset(output_buff, 0, test_tree_size);
	if(!database::pool::get_proxy()->blob_read(Blob, output_buff, test_tree_size, 0)){
		LOGGER; exit(1);
	}

	//make sure the tree we read, is identical to the tree we wrote.
	if(std::memcmp(input_buff, output_buff, test_tree_size) != 0){
		LOGGER; exit(1);
	}

	//remove the tree and make sure it no longer exists
	database::table::hash::delete_tree("ABC", test_tree_size);
	if(database::table::hash::exists("ABC", test_tree_size)){
		LOGGER; exit(1);
	}
}
