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
#include <p2p.hpp>

//standard
#include <string>
#include <vector>

class share;
class slot : private boost::noncopyable
{
	//only share can instantiate slot, this helps enforce uniqueness
	friend class share;
public:
	/* Info
	complete:
		Returns true if both the Hash_Tree and File are complete.
	file_size:
		Returns file size (bytes) or 0 if file size not known.
	hash:
		Returns hash file is tracked by.
	name:
		Returns the name of the file.
	path:
		Returns full path to file slot represents.
	*/
	bool complete();
	boost::uint64_t file_size();
	const std::string & hash();
	const std::string & name();
	std::string path();

	/* Other
	get_transfer:
		Get object responsible for exchanging hash tree and file data.
		Note: May be empty shared_ptr if we don't yet know the file size.
	info:
		Returns information that the GUI will need.
	set_unknown:
		Sets data that may not be known. Instantiates transfer. Returns false if
		transfer could not be instantiated.
	*/
	const boost::shared_ptr<transfer> & get_transfer();
	p2p::transfer info();
	bool set_unknown(const int connection_ID, const boost::uint64_t file_size,
		const std::string & root_hash);

private:
	//FI_in.file_size = 0 if we don't know file_size
	slot(const file_info & FI_in);

	const file_info FI;
	const std::string file_name;

	/*
	Instantiated when we get root_hash and file_size.
	Note: Mutex used to save on disk access. Blocks threads while one thread sets
		the root_hash and file_size.
	*/
	boost::mutex Transfer_mutex;
	boost::shared_ptr<transfer> Transfer;
};
#endif
