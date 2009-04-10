//THREADSAFE
#ifndef H_DATABASE_TABLE_HASH
#define H_DATABASE_TABLE_HASH

//boost
#include <boost/thread.hpp>

//custom
#include "path.hpp"
#include "settings.hpp"

//include
#include <database_connection.hpp>

//std
#include <sstream>

namespace database{
namespace table{
class hash
{
public:
	hash();

	enum state{
		DNE,         //indicates that no tree with key exists
		RESERVED,    //0 - reserved, any rows with this state are deleted on program start
		DOWNLOADING, //1 - downloading, incomplete tree
		COMPLETE     //2 - complete, hash tree complete and checked
	};

	/*
	delete_tree:
		Delete entry for tree of specified hash and tree_size.
	exists:
		Returns true with specified info exists.
	get_state:
		Returns state of hash tree (see state enum above).
	tree_allocate:
		Allocates space for tree with specified hash and size. The state of the
		tree is set to 0. The set_state function must be called for the hash tree
		to not be deleted on next program start.
	*/
	void delete_tree(const std::string & hash, const int & tree_size);
	static void delete_tree(const std::string & hash, const int & tree_size, database::connection & DB);
	bool exists(const std::string & hash, const int & tree_size);
	static bool exists(const std::string & hash, const int & tree_size, database::connection & DB);
	state get_state(const std::string & hash, const int & tree_size);
	static state get_state(const std::string & hash, const int & tree_size, database::connection & DB);
	void set_state(const std::string & hash, const int & tree_size, const state & State);
	static void set_state(const std::string & hash, const int & tree_size, const state & State, database::connection & DB);
	bool tree_allocate(const std::string & hash, const int & tree_size);
	static bool tree_allocate(const std::string & hash, const int & tree_size, database::connection & DB);

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
