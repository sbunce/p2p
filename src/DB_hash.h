#ifndef H_DB_HASH
#define H_DB_HASH

//boost
#include <boost/thread/mutex.hpp>

//custom
#include "global.h"
#include "sqlite3_wrapper.h"

//std
#include <sstream>

class DB_hash
{
public:
	/*
	This ctor is provided to let a DB_hash instantiation share a database
	connection. This is needed to be accessed within transactions involving
	other tables.
	*/
	DB_hash(sqlite3_wrapper::database & DB_in);

	enum state{
		DNE,         //indicates that no tree with key exists
		RESERVED,    //0 - reserved, any rows with this state are deleted on program start
		DOWNLOADING, //1 - downloading, incomplete tree
		COMPLETE     //2 - complete, hash tree complete and checked
	};

	/*
	delete_tree   - deletes row with the specified key
	exists        - returns true if tree with specified root hash and size exists
	get_key       - returns key necessary to open blob
	get_state     - returns the state of the hash tree (see enum state)
	tree_allocate - allocates space for tree of specified size and sets state = 0 (remember to call use!)
	tree_open     - returns a blob object that can be used to read/write a blob
	tree_use      - marks tree as used so it doesn't get deleted on startup
	*/
	void delete_tree(const std::string & hash, const int & tree_size);
	bool exists(const std::string & hash, const int & tree_size);
	state get_state(const std::string & hash, const int & tree_size);
	void set_state(const std::string & hash, const int & tree_size, const state & State);
	void tree_allocate(const std::string & hash, const int & tree_size);

	//opens/returns a blob to the tree with the specified hash and size
	static sqlite3_wrapper::blob tree_open(const std::string & hash, const int & tree_size)
	{
		sqlite3_wrapper::database DB;
		std::pair<bool, boost::int64_t> info;
		std::stringstream ss;
		ss << "SELECT key FROM hash WHERE hash = '" << hash << "' AND size = " << tree_size;
		DB.query(ss.str(), &get_key_call_back, info);
		if(info.first){
			return sqlite3_wrapper::blob("hash", "tree", info.second);
		}else{
			/*
			No tree exists in database that corresponds to hash. This is not
			necessarily and error. It could be that the tree only has 1 hash, the
			root hash, which is not contained within a hash tree.

			However, if this is an error and someone attempts to read/write to this
			returned blob then the program will be terminated.
			*/
			return sqlite3_wrapper::blob("hash", "tree", 0);
		}
	}

private:
	sqlite3_wrapper::database * DB;
	static bool program_start;
	static boost::mutex program_start_mutex;

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

	/*
	get_key - returns key of row with specified hash
	*/
	boost::int64_t get_key(const std::string & hash);
};
#endif
