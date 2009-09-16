#ifndef H_SLOT_ELEMENT
#define H_SLOT_ELEMENT

//custom
#include "database.hpp"
#include "file.hpp"
#include "file_info.hpp"
#include "hash_tree.hpp"

//include
#include <atomic_bool.hpp>
#include <boost/cstdint.hpp>
#include <boost/utility.hpp>

//standard
#include <string>

class share;
class slot : private boost::noncopyable
{
	/*
	Slots are unique (there exists only one slot per hash). Only the share class
	is able to instantiate a slot. This makes it easy to centrally enforce this.
	*/
	friend class share;
public:
	//all access to Hash_Tree must be locked with Hash_Tree_mutex
	boost::mutex Hash_Tree_mutex;
	hash_tree Hash_Tree;

	//all access to File must be locked with File_mutex
	boost::mutex File_mutex;
	file File;

	//all the hosts known to have the file
	boost::mutex host_mutex;
	std::set<database::table::host::host_info> host;

	const std::string & hash;
	const boost::uint64_t & tree_size;
	const boost::uint64_t & file_size;

private:
	slot(
		const file_info & FI
	):
		Hash_Tree(FI),
		File(FI),
		hash(Hash_Tree.hash),
		tree_size(Hash_Tree.tree_size),
		file_size(Hash_Tree.file_size)
	{}
};
#endif
