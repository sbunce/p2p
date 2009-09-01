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

class slot_element_set;
class slot_element : private boost::noncopyable
{
	/*
	A invariant of the slot_element is that it's unique (no other slot_element
	exists with the same hash). Only allowing the slot_element_set to instantiate
	the slot_elements makes it easy to centally enforce this invariant.
	*/
	friend class slot_element_set;

public:
	//all access to Hash_Tree must be locked with Hash_Tree_mutex
	boost::mutex Hash_Tree_mutex;
	hash_tree Hash_Tree;

	//all access to File must be locked with File_mutex
	boost::mutex File_mutex;
	file File;

	//convenience reference
	const std::string & hash;

private:
	slot_element(
		const std::string & hash,
		const boost::uint64_t & file_size,
		const std::string & path_in
	):
		Hash_Tree(hash, file_size),
		File(hash, file_size),
		hash(Hash_Tree.hash)
	{}
};
#endif
