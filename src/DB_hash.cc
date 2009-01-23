#include "DB_hash.h"

bool DB_hash::program_start(true);
boost::mutex DB_hash::program_start_mutex;

DB_hash::DB_hash(database & DB_in)
: DB(&DB_in)
{
	DB->query("CREATE TABLE IF NOT EXISTS hash(key INTEGER PRIMARY KEY, hash TEXT, state INTEGER, size INTEGER, tree BLOB)");
	DB->query("CREATE INDEX IF NOT EXISTS hash_hash_index ON hash(hash)");
	DB->query("CREATE INDEX IF NOT EXISTS hash_state_index ON hash(state)");
	DB->query("CREATE INDEX IF NOT EXISTS hash_size_index ON hash(size)");

	boost::mutex::scoped_lock lock(program_start_mutex);
	if(program_start){
		DB->query("DELETE FROM hash WHERE state = 0");
		program_start = false;
	}
}

void DB_hash::delete_tree(const std::string & hash, const int & tree_size)
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

bool DB_hash::exists(const std::string & hash, const int & tree_size)
{
	bool tree_exists = false;
	std::stringstream ss;
	ss << "SELECT 1 FROM hash WHERE hash = '" << hash << "' AND size = " << tree_size;
	DB->query(ss.str(), &exists_call_back, tree_exists);
	return tree_exists;
}

boost::int64_t DB_hash::get_key(const std::string & hash)
{
	std::pair<bool, boost::int64_t> info;
	std::stringstream ss;
	ss << "SELECT key FROM hash WHERE hash = '" << hash << "'";
	DB->query(ss.str(), &get_key_call_back, info);
	if(info.first){
		return info.second;
	}else{
		LOGGER << "no key found for " << hash;
		exit(1);
	}
}

static int get_state_call_back(std::pair<bool, unsigned> & info, int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0]);
	info.first = true;
	std::stringstream ss;
	ss << response[0];
	ss >> info.second;
	return 0;
}

DB_hash::state DB_hash::get_state(const std::string & hash, const int & tree_size)
{
	//std::pair<true if found, key>
	std::pair<bool, unsigned> info;
	std::stringstream ss;
	ss << "SELECT state FROM hash WHERE hash = '" << hash << "' AND size = " << tree_size;
	DB->query(ss.str(), &get_state_call_back, info);
	if(info.first){
		if(info.second == 0){
			return DB_hash::RESERVED;
		}else if(info.second == 1){
			return DB_hash::DOWNLOADING;
		}else if(info.second == 2){
			return DB_hash::COMPLETE;
		}else{
			LOGGER << "unknown state in database";
			exit(1);
		}
	}else{
		return DB_hash::DNE;
	}
}

void DB_hash::set_state(const std::string & hash, const int & tree_size, const state & State)
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
	DB->query(ss.str());
}

void DB_hash::tree_allocate(const std::string & hash, const int & tree_size)
{
	std::stringstream ss;
	ss << "INSERT INTO hash(key, hash, state, size, tree) VALUES(NULL, '" << hash << "', 0, " << tree_size << ", ?)";
	DB->blob_allocate(ss.str(), tree_size);
}
