//THREADSAFE
#ifndef H_DATABASE_TABLE_HASH
#define H_DATABASE_TABLE_HASH

//custom
#include "database.hpp"
#include "path.hpp"
#include "settings.hpp"

//include
#include <boost/thread.hpp>

//standard
#include <sstream>

namespace database{
namespace table{
class hash
{
public:
	enum state{
		RESERVED,    //0 - reserved, (deleted upon program start)
		DOWNLOADING, //1 - downloading, incomplete tree
		COMPLETE     //2 - complete, hash tree complete and checked
	};

	/*
	clear:
		Clears the entire hash table.
	delete_tree:
		Delete entry for tree of specified hash and tree_size.
	exists:
		Returns true with specified info exists.
	get_state:
		Returns state of hash tree (see state enum above).
	tree_allocate:
		Allocates space for tree with specified hash and size. The set_state
		function must be called for the hash tree to not be deleted on next
		program start.
	*/
	static void clear();
	static void delete_tree(const std::string & hash, const int tree_size);
	static bool exists(const std::string & hash, const int tree_size);
	static bool get_state(const std::string & hash, const int tree_size, state & State);
	static void set_state(const std::string & hash, const int tree_size, const state State);
	static bool tree_allocate(const std::string & hash, const int tree_size);
	static database::blob tree_open(const std::string & hash, const int tree_size);

	/* Connection Functions
	All these functions correspond to above functions but they take a database
	connection. These are needed to do calls to multiple database functions
	within a transaction.
	*/
	static void clear(database::pool::proxy & DB);
	static void delete_tree(const std::string & hash, const int tree_size, database::pool::proxy & DB);
	static bool exists(const std::string & hash, const int tree_size, database::pool::proxy & DB);
	static bool get_state(const std::string & hash, const int tree_size, state & State, database::pool::proxy & DB);
	static void set_state(const std::string & hash, const int tree_size, const state State, database::pool::proxy & DB);
	static bool tree_allocate(const std::string & hash, const int tree_size, database::pool::proxy & DB);
	static database::blob tree_open(const std::string & hash, const int tree_size, database::pool::proxy & DB);

private:
	hash(){}
};
}//end of namespace table
}//end of namespace database
#endif
