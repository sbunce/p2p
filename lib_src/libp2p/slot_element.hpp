#ifndef H_SLOT_ELEMENT
#define H_SLOT_ELEMENT

//custom
#include "file.hpp"
#include "hash_tree.hpp"

//include
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
		File(hash, file_size),
		Hash_Tree(hash, file_size)
	{}

	//all access to File must be locked with File_mutex
	boost::mutex File_mutex;
	file File;

	//all access to Hash_Tree must be locked with Hash_Tree_mutex
	boost::mutex Hash_Tree_mutex;
	hash_tree Hash_Tree;
};
#endif
