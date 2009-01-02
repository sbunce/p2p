#ifndef H_DB_HASH_FREE
#define H_DB_HASH_FREE

//boost
#include <boost/thread/mutex.hpp>

//custom
#include "atomic_bool.h"
#include "global.h"
#include "sqlite3_wrapper.h"

//std
#include <string>

class DB_hash_free
{
public:
	DB_hash_free();
	/*
	Locates a free space in the hash tree file large enough to accomodate a hash
	tree of the given tree_size (bytes). The space found is reserved. All space is
	unreserved when the program starts.

	returns std::pair<offset, size> allocated
	*/
	static std::pair<boost::uint64_t, boost::uint64_t> find_and_reserve(const boost::uint64_t tree_size)
	{

	}

	/*
	Use space reserved by find_and_reserve(). This function removes the reserved
	space from the free list.

	info is the std::pair returned from find_and_reserve()
	*/
	static void use(const std::pair<boost::uint64_t, boost::uint64_t> & info)
	{

	}

private:
	sqlite3_wrapper DB;

	static atomic_bool program_start;
};
#endif
