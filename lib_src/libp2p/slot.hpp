#ifndef H_SLOT
#define H_SLOT

//custom
#include "db_all.hpp"
#include "file_info.hpp"
#include "transfer.hpp"

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
	complete:
		Returns true if both the Hash_Tree and File are complete.
	download_peers:
		Returns how many hosts we're downloading this file from.
	download_speed:
		Returns download speed (bytes/second).
	file_size:
		Returns file size (bytes). If returned file size is 0 it means we don't
		yet know the file size.
	hash:
		Returns hash file is tracked by.
	name:
		Returns the name of the file.
	path:
		Returns full path to file slot represents.
	percent_complete:
		Returns percent complete of the download (0-100).
	upload_peers:
		Returns how many hosts we're uploading this file to.
	upload_speed:
		Returns upload speed (bytes/second).
	*/
//DEBUG, get rid of most of these
	bool complete();
	unsigned download_peers();
	unsigned download_speed();
	boost::uint64_t file_size();
	const std::string & hash();
	const std::string & name();
	std::string path();
	unsigned percent_complete();
	unsigned upload_peers();
	unsigned upload_speed();

	/* Other
	get_transfer:
		Get object responsible for exchanging hash tree and file data.
		Note: May be empty shared_ptr if we don't yet know the file size.
	set_unknown:
		Sets data that may not be known. Instantiates transfer. Returns false if
		transfer could not be instantiated.
	touch:
		Updates speeds. Called before getting speeds to return to the GUI.
	*/
	const boost::shared_ptr<transfer> & get_transfer();
	bool set_unknown(const int connection_ID, const boost::uint64_t file_size,
		const std::string & root_hash);
	void touch();

private:
	/*
	Note: The ctor may throw.
	Note: If FI_in.file_size = 0 it means we don't know the file size.
	*/
	slot(const file_info & FI_in);

	const file_info FI;
	const std::string file_name;

	//instantiated when we get file size
	boost::mutex Transfer_mutex; //lock for instantiation of transfer
	boost::shared_ptr<transfer> Transfer;
};
#endif
