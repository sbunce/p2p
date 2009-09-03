#ifndef H_SLOT_ELEMENT
#define H_SLOT_ELEMENT

//custom
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
	/*
	A invariant of the slot_element is that it's unique (no other slot_element
	exists with the same hash). Only allowing the slot_element_set to instantiate
	the slot_elements makes it easy to centally enforce this invariant.
	*/
	friend class slot_set;

public:
	//all access to Hash_Tree must be locked with Hash_Tree_mutex
	boost::mutex Hash_Tree_mutex;
	hash_tree Hash_Tree;

	//all access to File must be locked with File_mutex
	boost::mutex File_mutex;
	file File;

	//convenience references
	const std::string & hash;
	const boost::uint64_t & tree_size;
	const boost::uint64_t & file_size;

private:
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

	/*
	All the hosts known to have the file.
	*/
	boost::mutex host_mutex;
	std::set<std::string> host;
};
#endif
