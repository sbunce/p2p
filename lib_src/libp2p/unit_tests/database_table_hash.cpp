//custom
#include "../database.hpp"

//standard
#include <cstring>

int main()
{
	//setup database and make sure hash table clear
	path::unit_test_override("database_table_hash.db");
	database::init::drop_all();
	database::init::create_all();

	//test tree info
	database::table::hash::tree_info TI;
	TI.hash = "ABC";
	TI.tree_size = 123;

	//tree has not been added yet, it should not exist
	if(database::table::hash::tree_open(TI.hash)){
		LOGGER; exit(1);
	}

	//add tree to table (allocate it)
	database::table::hash::tree_allocate(TI.hash, TI.tree_size);

	//we just added the tree so it should exist
	if(boost::shared_ptr<database::table::hash::tree_info>
		TI_ptr = database::table::hash::tree_open(TI.hash))
	{
		if(TI_ptr->hash != TI.hash){ LOGGER; exit(1); }
		if(TI_ptr->tree_size != TI.tree_size){ LOGGER; exit(1); }
		if(TI_ptr->State != database::table::hash::reserved){ LOGGER; exit(1); }
	}else{
		LOGGER; exit(1);
	}

	//test changing the state to complete
	database::table::hash::set_state(TI.hash, database::table::hash::complete);

	//make sure the state is no complete
	if(boost::shared_ptr<database::table::hash::tree_info>
		TI_ptr = database::table::hash::tree_open(TI.hash))
	{
		if(TI_ptr->State != database::table::hash::complete){ LOGGER; exit(1); }
	}else{
		LOGGER; exit(1);
	}

	//test reading/writing the tree
	if(boost::shared_ptr<database::table::hash::tree_info>
		TI_ptr = database::table::hash::tree_open(TI.hash))
	{
		//turn on every bit in the tree to test
		char * input_buff = static_cast<char *>(std::malloc(TI_ptr->tree_size));
		std::memset(input_buff, 255, TI_ptr->tree_size);
		if(!database::pool::get_proxy()->blob_write(TI_ptr->Blob, input_buff,
			TI_ptr->tree_size, 0))
		{
			LOGGER; exit(1);
		}

		//read the tree
		char * output_buff = static_cast<char *>(std::malloc(TI_ptr->tree_size));
		std::memset(output_buff, 0, TI_ptr->tree_size);
		if(!database::pool::get_proxy()->blob_read(TI_ptr->Blob, output_buff,
			TI_ptr->tree_size, 0))
		{
			LOGGER; exit(1);
		}

		//make sure the tree we read, is identical to the tree we wrote.
		if(std::memcmp(input_buff, output_buff, TI_ptr->tree_size) != 0){
			LOGGER; exit(1);
		}
	}else{
		LOGGER; exit(1);
	}

	//remove the tree
	database::table::hash::delete_tree(TI.hash);

	//the tree should no longer exist since it was removed
	if(boost::shared_ptr<database::table::hash::tree_info>
		TI_ptr = database::table::hash::tree_open(TI.hash))
	{
		LOGGER; exit(1);
	}
}
