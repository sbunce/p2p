#ifndef H_SLOT_ELEMENT
#define H_SLOT_ELEMENT

//custom
#include "database.hpp"
#include "file.hpp"
#include "hash_tree.hpp"

//include
#include <atomic_bool.hpp>
#include <boost/cstdint.hpp>
#include <boost/utility.hpp>

//standard
#include <string>

class slot_set;
class slot : private boost::noncopyable
{
private:
	/*
	Each slot that exists must be unique (no other slot exists for a file with
	the same hash). Only allowing the slot_manager to instantiate a slot makes
	it easy to centrally enforce this.
	*/
	friend class slot_set;

	slot(
		const std::string & hash,
		const boost::uint64_t & file_size,
		const std::string & path_in
	):
		Hash_Tree(hash, file_size),
		File(hash, file_size),
		hash(Hash_Tree.hash),
		tree_size(Hash_Tree.tree_size),
		file_size(File.file_size)
	{}

	//all access to Hash_Tree must be locked with Hash_Tree_mutex
	boost::mutex Hash_Tree_mutex;
	hash_tree Hash_Tree;

	//all access to File must be locked with File_mutex
	boost::mutex File_mutex;
	file File;

	//all the hosts known to have the file
	boost::mutex host_mutex;
	std::set<database::table::host::host_info> host;

public:
	const std::string & hash;
	const boost::uint64_t & tree_size;
	const boost::uint64_t & file_size;
};
#endif
