#ifndef H_SLOT
#define H_SLOT

//custom
#include "database.hpp"
#include "file.hpp"
#include "file_info.hpp"
#include "hash_tree.hpp"

//include
#include <atomic_bool.hpp>
#include <boost/cstdint.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

//standard
#include <string>
#include <vector>

class share;
class slot : private boost::noncopyable
{
	/*
	Slots are unique (there exists only one slot per hash). Only the share class
	is able to instantiate a slot. This makes it easy to centrally enforce
	uniqueness.
	*/
	friend class share;
public:

	hash_tree Hash_Tree;
	file File;

	/* Info Related Functions
	complete:
		Returns true if both the Hash_Tree and File are complete.
	has_file:
		Returns true if the host has the file associated with this slot.
	merge_host:
		Inserts all hosts known to have this file in to host_in.
	*/
	bool complete();
	bool has_file(const std::string & IP, const std::string & port);
	void merge_host(std::set<std::pair<std::string, std::string> > & host_in);

private:
	//the ctor will throw an exception if database access fails
	slot(
		const file_info & FI,
		database::pool::proxy DB = database::pool::proxy()
	);

	//all the hosts known to have the file
	boost::mutex host_mutex;
	std::set<std::pair<std::string, std::string> > host;
};
#endif
