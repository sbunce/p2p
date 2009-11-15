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
	is able to instantiate a slot. This makes it easy to enforce uniqueness.
	*/
	friend class share;

public:

	/* Info (info to return to GUI)
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

	/* Resume (used in p2p_real to resume downloads)
	check:
		Hash checks the hash tree and the file if the hash tree is complete.
		Note: This function takes a long time to run.
	*/
	void check();

	/* Slot Manager (functions slot_manager uses)
	available:
		Returns false if we don't yet have the root hash and file size for the
		file. Returns true if we can open the slot.
	complete:
		Returns true if both the Hash_Tree and File are complete.
	has_file:
		Returns true if the host has the file associated with this slot.
	merge_host:
		Inserts all hosts known to have this file in to host_in.
	set_missing:
		Sets the root hash and file size (these are gotten when doing a slot
		request). If the hash_tree and file are instantiated by this function if
		they aren't already.
	status:
		Returns the status byte for a SLOT_ID message (see protocol doc).
		Precondition: available() = true
	*/
	bool available();
	bool complete();
	bool has_file(const std::string & IP, const std::string & port);
	void merge_host(std::set<std::pair<std::string, std::string> > & host_in);
	void set_missing(const std::string & root_hash, const boost::uint64_t file_size);
	unsigned char status();

private:
	//the ctor will throw an exception if database access fails
	slot(const file_info & FI_in);

	//before hash_tree and file instantiated info stored here
	file_info FI;

	//instantiated when we have root hash and file size
	boost::shared_ptr<hash_tree> Hash_Tree;
	boost::shared_ptr<file> File;

	//all the hosts known to have the file
	boost::mutex host_mutex;
	std::set<std::pair<std::string, std::string> > host;
};
#endif
