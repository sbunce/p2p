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
	hash();
	hash(database::connection & DB_in);

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
	bool exists(const std::string & hash, const int & tree_size);
	state get_state(const std::string & hash, const int & tree_size);
	void set_state(const std::string & hash, const int & tree_size, const state & State);
	void tree_allocate(const std::string & hash, const int & tree_size);

	/*
	Opens/returns a blob to the tree with the specified hash and size. The version
	that doesn't have the database connection as the first parameter creates it's
	own temporary database connection. It's preferable to use the one with the
	database connection parameter but if that is not convenient the other is
	provided.
	*/
	static database::blob tree_open(const std::string & hash, const int & tree_size);
	static database::blob tree_open(database::connection & DB, const std::string & hash, const int & tree_size);

private:
	database::connection * DB;

	//used by ctor to run something upon first instantiation
	static bool program_start;
	static boost::mutex program_start_mutex;

	/*
	get_key - returns key of row with specified hash
	*/
	boost::int64_t get_key(const std::string & hash);
};
}//end of table namespace
}//end of database namespace
#endif
