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

class slot_element : private boost::noncopyable
{
public:
	slot_element(
		const std::string & hash,
		const boost::uint64_t & file_size
	):
		Hash_Tree(hash, file_size),
		File(hash, file_size),
		hash(Hash_Tree.hash)
	{}

	/*
	When this flag is true it means the hash_tree and/or file will be or are
	being hash checked. This happens on program start when downloads are resumed.

	During hash checking the slot_element will not leave the slot_element_set and
	the Hash_Tree and File will not be accessed by anyone but the hashing thread.
	*/
	atomic_bool hash_checking;

	//all access to Hash_Tree must be locked with Hash_Tree_mutex
	boost::mutex Hash_Tree_mutex;
	hash_tree Hash_Tree;

	//all access to File must be locked with File_mutex
	boost::mutex File_mutex;
	file File;

	//convenience reference
	const std::string & hash;
};
#endif
