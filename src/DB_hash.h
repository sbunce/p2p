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
	The DB hash table will need to be accessed within transactions involving
	other tables. For this reason it doesn't have its own instantation of the
	sqlite3_wrapper.
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
	get_state     - returns the state of the hash tree (see enum state)
	tree_allocate - allocates space for tree of specified size and sets state = 0 (remember to call use!)
	tree_open     - returns a blob object that can be used to read/write a blob
	tree_use      - marks tree as used so it doesn't get deleted on startup
	*/
	void delete_tree(const boost::int64_t & key);
	state get_state(const boost::int64_t & key);
	void set_state(const boost::int64_t & key, const state & State);
	boost::int64_t tree_allocate(const int & size);
	sqlite3_wrapper::blob tree_open(const boost::int64_t & key);
	void tree_use(const boost::int64_t & key);

private:
	sqlite3_wrapper::database * DB;
	static bool program_start;
	static boost::mutex program_start_mutex;
};
#endif
