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
#include <cstring>
#include <sstream>

namespace database{
namespace table{
class hash_tree
{
public:
	enum state{
		reserved,    //record reserved but not used (delete on next program start)
		downloading, //tree or file incomplete
		complete     //tree or file complete (all blocks hash checked good)
	};

	class info
	{
	public:
		std::string hash;
		boost::uint64_t tree_size;
		state tree_state;
		database::blob Blob;
	};

	/*
	add:
		Allocates blob of specified size for hash tree. The set_state function
		must be called for the hash tree to not be deleted on next program start.
		False returned if error allocating space.
	lookup:
		Returns a shared_ptr with information for the tree. Empty shared_ptr is
		returned if the tree doesn't exist.
	remove:
		Remove the hash tree with the specified hash.
	set_state:
		Sets the state of the hash tree.
	*/
	static bool add(const std::string & hash, const boost::uint64_t & tree_size,
		database::pool::proxy DB = database::pool::proxy());
	static boost::shared_ptr<info> lookup(const std::string & hash,
		database::pool::proxy DB = database::pool::proxy());
	static void remove(const std::string & hash,
		database::pool::proxy DB = database::pool::proxy());
	static void set_state(const std::string & hash, const state tree_state,
		database::pool::proxy DB = database::pool::proxy());
private:
	hash_tree(){}
};
}//end of namespace table
}//end of namespace database
#endif
