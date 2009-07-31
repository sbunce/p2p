#include "database_table_hash.hpp"

void database::table::hash::clear(database::pool::proxy DB)
{
	DB->query("DELETE FROM hash");
}

void database::table::hash::delete_tree(const std::string & hash,
	const int tree_size, database::pool::proxy DB)
{
	std::stringstream ss;
	ss << "DELETE FROM hash WHERE hash = '" << hash << "' AND size = " << tree_size;
	DB->query(ss.str());
}

static int exists_call_back(bool & tree_exists, int columns_retrieved,
	char ** response, char ** column_name)
{
	assert(response[0]);
	tree_exists = true;
	return 0;
}

bool database::table::hash::exists(const std::string & hash,
	const int tree_size, database::pool::proxy DB)
{
	bool tree_exists = false;
	std::stringstream ss;
	ss << "SELECT 1 FROM hash WHERE hash = '" << hash << "' AND size = "
		<< tree_size;
	DB->query(ss.str(), &exists_call_back, tree_exists);
	return tree_exists;
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

bool database::table::hash::get_state(const std::string & hash,
	const int tree_size, state & State, database::pool::proxy DB)
{
	//std::pair<true if found, key>
	std::pair<bool, unsigned> info(false, 0);
	std::stringstream ss;
	ss << "SELECT state FROM hash WHERE hash = '" << hash << "' AND size = "
		<< tree_size;
	DB->query(ss.str(), &get_state_call_back, info);
	if(info.first){
		assert(info.second >= RESERVED && info.first <= COMPLETE);
		State = (state)info.second;
		return true;
	}else{
		return false;
	}
}

void database::table::hash::set_state(const std::string & hash,
	const int tree_size, const state State, database::pool::proxy DB)
{
	assert(State >= RESERVED && State <= COMPLETE);
	std::stringstream ss;
	ss << "UPDATE hash SET state = " << State << " WHERE hash = '"
		<< hash << "' AND size = " << tree_size;
	DB->query(ss.str());
}

bool database::table::hash::tree_allocate(const std::string & hash,
	const int tree_size, database::pool::proxy DB)
{
	if(tree_size != 0){
		std::stringstream ss;
		ss << "INSERT INTO hash(key, hash, state, size, tree) VALUES(NULL, '"
			<< hash << "', 0, " << tree_size << ", ?)";
		return DB->blob_allocate(ss.str(), tree_size);
	}else{
		return false;
	}
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

database::blob database::table::hash::tree_open(const std::string & hash,
	const int tree_size, database::pool::proxy DB)
{
	std::pair<bool, boost::int64_t> info(false, 0);
	std::stringstream ss;
	ss << "SELECT key FROM hash WHERE hash = '" << hash << "' AND size = "
		<< tree_size;
	DB->query(ss.str(), &get_key_call_back, info);
	if(info.first){
		return database::blob("hash", "tree", info.second);
	}else{
		/*
		No tree exists in database that corresponds to hash. This is not
		necessarily an error. It could be that the tree only has 1 hash, the
		root hash, which is not contained within a hash tree.

		However, if this is an error and someone attempts to read/write to this
		returned blob then the program will be terminated.
		*/
		return database::blob("hash", "tree", 0);
	}
}
