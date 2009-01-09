#ifndef H_DB_HASH
#define H_DB_HASH

//boost
#include <boost/tuple/tuple.hpp>

//custom
#include "atomic_bool.h"
#include "global.h"
#include "sqlite3_wrapper.h"

//std
#include <string>

class DB_hash
{
public:
	DB_hash();

	//does memmove like operations to compact HASH file
	void compact();

	/*
	Will de-allocate space that was used for a hash tree.
	*/
	void free(const std::string & hash);

	/*
	Locates a free space in the HASH file large enough to accomodate a hash
	tree of the given tree_size (bytes). The return value is an offset from the
	start of the file (bytes) at which to start writing the hash tree.
	*/
	boost::uint64_t reserve(const boost::uint64_t tree_size);

	/*
	Use a space that was reserve()'ed.
	precondition: the space pointed to by offset must have reserved
	*/
	void use(const boost::uint64_t & offset, const std::string & hash);

private:
	sqlite3_wrapper DB;
	static atomic_bool program_start;

	/*
	best_fix_call_back     - call back used by find_and_reserve() to determine best fit
	create_space_call_back - call back used by find_and_reserve() to allocate space
	use_call_back          - call back used by use() to change state to used
	*/
	int best_fit_call_back(std::pair<bool, boost::uint64_t> & info,
		int columns_retrieved, char ** response, char ** column_name);
	int create_space_call_back(std::pair<bool, boost::uint64_t> & info,
		int columns_retrieved, char ** response, char ** column_name);
	int use_call_back(boost::tuple<bool, const boost::uint64_t *, const std::string *> & info,
		int columns_retrieved, char ** response, char ** column_name);
};
#endif
