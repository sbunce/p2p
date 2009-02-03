//THREADSAFE
#ifndef H_DATABASE_TABLE_HASH
#define H_DATABASE_TABLE_HASH

//boost
#include <boost/thread.hpp>

//custom
#include "database_connection.hpp"
#include "global.hpp"

//std
#include <sstream>

namespace database{
namespace table{
class hash
{
public:
	hash(){}

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
	tree_use      - marks tree as used so it doesn't get deleted on startup
	*/
	void delete_tree(const std::string & hash, const int & tree_size);
	static void delete_tree(const std::string & hash, const int & tree_size, database::connection & DB);
	bool exists(const std::string & hash, const int & tree_size);
	static bool exists(const std::string & hash, const int & tree_size, database::connection & DB);
	state get_state(const std::string & hash, const int & tree_size);
	static state get_state(const std::string & hash, const int & tree_size, database::connection & DB);
	void set_state(const std::string & hash, const int & tree_size, const state & State);
	static void set_state(const std::string & hash, const int & tree_size, const state & State, database::connection & DB);
	void tree_allocate(const std::string & hash, const int & tree_size);
	static void tree_allocate(const std::string & hash, const int & tree_size, database::connection & DB);

	/*
	Opens/returns a blob to the tree with the specified hash and size. The first
	static version creates it's own database connection. This is used in contexts
	where it's inconvenient to instantiate a DB connection or download::table::hash,
	such as in hash_tree::tree_info.
	*/
	static database::blob tree_open(const std::string & hash, const int & tree_size);
	static database::blob tree_open(const std::string & hash, const int & tree_size, database::connection & DB);

private:
	database::connection DB;
};
}//end of table namespace
}//end of database namespace
#endif
