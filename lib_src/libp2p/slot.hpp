#ifndef H_SLOT
#define H_SLOT

//custom
#include "database.hpp"
#include "exchange.hpp"
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
	is able to instantiate a slot. This makes it easy to enforce uniqueness.
	*/
	friend class share;

public:
	/* Info
	download_speed:
		Returns download speed (bytes/second).
	file_size:
		Returns file size (bytes).
	hash:
		Returns hash file is tracked by.
	name:
		Returns the name of the file.
	percent_complete:
		Returns percent complete of the download (0-100).
	upload_speed:
		Returns upload speed (bytes/second).
	*/
	unsigned download_speed();
	boost::uint64_t file_size();
	const std::string & hash();
	std::string name();
	unsigned percent_complete();
	unsigned upload_speed();

	/* Resume
	check:
		Hash checks the hash tree and the file if the hash tree is complete.
		Note: This function takes a long time to run.
	*/
	void check();

	/* Slot Manager
	complete:
		Returns true if both the Hash_Tree and File are complete.
	create_REQUEST_SLOT:
		Creates a REQUEST_SLOT message with the given slot_num.
	*/
	bool complete();
	boost::shared_ptr<exchange::message> create_request_slot(const unsigned char slot_num);

private:
	//the ctor will throw an exception if database access fails
	slot(const file_info & FI);

	hash_tree Hash_Tree;
	file File;
};
#endif
