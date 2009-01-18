#include "DB_hash.h"

bool DB_hash::program_start(true);
boost::mutex DB_hash::program_start_mutex;


DB_hash::DB_hash(sqlite3_wrapper::database & DB_in)
: DB(&DB_in)
{
	DB->query("CREATE TABLE IF NOT EXISTS hash(key INTEGER PRIMARY KEY, state INTEGER, tree BLOB)");

	boost::mutex::scoped_lock lock(program_start_mutex);
	if(program_start){
		DB->query("DELETE FROM hash WHERE state = 0");
		program_start = false;
	}
}

void DB_hash::delete_tree(const boost::int64_t & key)
{
	std::stringstream query;
	query << "DELETE FROM hash WHERE key = " << key;
	DB->query(query.str());
}

static int get_state_call_back(std::pair<bool, boost::int64_t> & info, int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0]);
	info.first = true;
	std::stringstream ss;
	ss << response[0];
	ss >> info.second;
	return 0;
}

DB_hash::state DB_hash::get_state(const boost::int64_t & key)
{
	//std::pair<true if found, key>
	std::pair<bool, boost::int64_t> info;
	std::stringstream ss;
	ss << "SELECT state FROM hash WHERE key = " << key;
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

void DB_hash::set_state(const boost::int64_t & key, const state & State)
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
	ss << " WHERE key = " << key;
	DB->query(ss.str());
}

boost::int64_t DB_hash::tree_allocate(const int & size)
{
	std::stringstream ss;
	ss << "INSERT INTO hash(key, state, tree) VALUES(NULL, 0, ?)";
	return DB->blob_allocate(ss.str(), size);
}

sqlite3_wrapper::blob DB_hash::tree_open(const boost::int64_t & key)
{
	return sqlite3_wrapper::blob("hash", "tree", key);
}

void DB_hash::tree_use(const boost::int64_t & key)
{
	std::stringstream query;
	query << "UPDATE hash SET state = 1 WHERE key = " << key;
	DB->query(query.str());
}
