#include "database_table_hash.hpp"

database::table::hash::hash():
	DB(path::database())
{

}

void database::table::hash::delete_tree(const std::string & hash,
	const int & tree_size)
{
	delete_tree(hash, tree_size, DB);
}

void database::table::hash::delete_tree(const std::string & hash,
	const int & tree_size, database::connection & DB)
{
	std::stringstream ss;
	ss << "DELETE FROM hash WHERE hash = '" << hash << "' AND size = " << tree_size;
	DB.query(ss.str());
}

static int exists_call_back(bool & tree_exists, int columns_retrieved,
	char ** response, char ** column_name)
{
	assert(response[0]);
	tree_exists = true;
	return 0;
}

bool database::table::hash::exists(const std::string & hash,
	const int & tree_size)
{
	return exists(hash, tree_size, DB);
}

bool database::table::hash::exists(const std::string & hash,
	const int & tree_size, database::connection & DB)
{
	bool tree_exists = false;
	std::stringstream ss;
	ss << "SELECT 1 FROM hash WHERE hash = '" << hash << "' AND size = "
		<< tree_size;
	DB.query(ss.str(), &exists_call_back, tree_exists);
	return tree_exists;
}

static int get_key_call_back(std::pair<bool, boost::int64_t> & info,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0]);
	info.first = true;
	std::stringstream ss;
	ss << response[0];
	ss >> info.second;
	return 0;
}

static int get_state_call_back(std::pair<bool, unsigned> & info,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0]);
	info.first = true;
	std::stringstream ss;
	ss << response[0];
	ss >> info.second;
	return 0;
}

database::table::hash::state database::table::hash::get_state(
	const std::string & hash, const int & tree_size)
{
	return get_state(hash, tree_size, DB);
}

database::table::hash::state database::table::hash::get_state(
const std::string & hash, const int & tree_size, database::connection & DB)
{
	//std::pair<true if found, key>
	std::pair<bool, unsigned> info;
	std::stringstream ss;
	ss << "SELECT state FROM hash WHERE hash = '" << hash << "' AND size = "
		<< tree_size;
	DB.query(ss.str(), &get_state_call_back, info);
	if(info.first){
		if(info.second == 0){
			return database::table::hash::RESERVED;
		}else if(info.second == 1){
			return database::table::hash::DOWNLOADING;
		}else if(info.second == 2){
			return database::table::hash::COMPLETE;
		}else{
			LOGGER << "unknown state in database";
			exit(1);
		}
	}else{
		return database::table::hash::DNE;
	}
}

void database::table::hash::set_state(const std::string & hash,
	const int & tree_size, const state & State)
{
	set_state(hash, tree_size, State, DB);
}

void database::table::hash::set_state(const std::string & hash,
	const int & tree_size, const state & State, database::connection & DB)
{
	std::stringstream ss;
	ss << "UPDATE hash SET state = ";
	if(State == RESERVED){
		ss << 0;
	}else if(State == DOWNLOADING){
		ss << 1;
	}else if(State == COMPLETE){
		ss << 2;
	}else{
		LOGGER << "unknown state";
		exit(1);
	}
	ss << " WHERE hash = '" << hash << "' AND size = " << tree_size;
	DB.query(ss.str());
}

bool database::table::hash::tree_allocate(const std::string & hash,
	const int & tree_size)
{
	return tree_allocate(hash, tree_size, DB);
}

bool database::table::hash::tree_allocate(const std::string & hash,
	const int & tree_size, database::connection & DB)
{
	std::stringstream ss;
	ss << "INSERT INTO hash(key, hash, state, size, tree) VALUES(NULL, '"
		<< hash << "', 0, " << tree_size << ", ?)";
	return DB.blob_allocate(ss.str(), tree_size);
}

database::blob database::table::hash::tree_open(const std::string & hash,
	const int & tree_size)
{
exit(1);
	//database::connection DB;
	//return tree_open(hash, tree_size, DB);
}

database::blob database::table::hash::tree_open(const std::string & hash,
	const int & tree_size, database::connection & DB)
{
	std::pair<bool, boost::int64_t> info;
	std::stringstream ss;
	ss << "SELECT key FROM hash WHERE hash = '" << hash << "' AND size = "
		<< tree_size;
	DB.query(ss.str(), &get_key_call_back, info);
	if(info.first){
		return database::blob("hash", "tree", info.second);
	}else{
		/*
		No tree exists in database that corresponds to hash. This is not
		necessarily and error. It could be that the tree only has 1 hash, the
		root hash, which is not contained within a hash tree.

		However, if this is an error and someone attempts to read/write to this
		returned blob then the program will be terminated.
		*/
		return database::blob("hash", "tree", 0);
	}
}
