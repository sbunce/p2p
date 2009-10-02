#ifndef H_DATABASE_TABLE_HASH
#define H_DATABASE_TABLE_HASH

//custom
#include "database.hpp"
#include "path.hpp"
#include "settings.hpp"

//include
#include <boost/lexical_cast.hpp>
#include <boost/ref.hpp>
#include <boost/thread.hpp>

//standard
#include <sstream>

namespace database{
namespace table{
class hash
{
public:
	enum state{
		reserved,    //0 - reserved, (deleted upon program start)
		downloading, //1 - downloading, incomplete tree
		complete     //2 - complete, hash tree complete and checked
	};

	class tree_info
	{
	public:
		std::string hash;
		boost::uint64_t tree_size;
		state State;
		database::blob Blob;
	};

	/*
	delete_tree:
		Delete record for tree with specified hash.
	get_state:
		Returns state of hash tree (see state enum above).
	tree_allocate:
		Allocates space for tree with specified hash and size. The set_state
		function must be called for the hash tree to not be deleted on next
		program start. True returned if space could be allocated or false if there
		is error.
	tree_open:
		Returns a shared_ptr with information for the tree. A empty shared_ptr is
		returned if the tree doesn't exist.
	*/
	static void delete_tree(const std::string & hash,
		database::pool::proxy DB = database::pool::proxy());
	static void set_state(const std::string & hash, const state State,
		database::pool::proxy DB = database::pool::proxy());
	static bool tree_allocate(const std::string & hash, const boost::uint64_t & tree_size,
		database::pool::proxy DB = database::pool::proxy());
	static boost::shared_ptr<tree_info> tree_open(const std::string & hash,
		database::pool::proxy DB = database::pool::proxy());

private:
	hash(){}
};
}//end of namespace table
}//end of namespace database
#endif
