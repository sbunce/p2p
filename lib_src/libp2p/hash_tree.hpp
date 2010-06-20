/*
The ctor will throw an exception if database access fails. The thread safety of
this class is exactly like a file.
*/
#ifndef H_HASH_TREE
#define H_HASH_TREE

//custom
#include "block_request.hpp"
#include "db_all.hpp"
#include "file_info.hpp"
#include "path.hpp"
#include "protocol_tcp.hpp"
#include "settings.hpp"
#include "tree_info.hpp"

//include
#include <atomic_int.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <convert.hpp>
#include <net/net.hpp>
#include <SHA1.hpp>
#include <thread_pool.hpp>

//standard
#include <deque>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

/*
The hash_tree object can be used to read/write, or reconstruct a hash tree.
*/
class hash_tree : private boost::noncopyable
{
public:
	hash_tree(
		const file_info & FI,
		db::pool::proxy DB = db::pool::proxy()
	);

	enum status{
		good,     //block is good
		bad,      //block is bad
		io_error, //error reading hash tree
		copying   //file may be copying (it is increasing in size)
	};

	//tree information needed to read/write tree
	const tree_info TI;

	/*
	check:
		Checks the entire hash tree. This is called on program start to see what
		blocks are good.
	check_block:
		Checks the hash tree block in buf.
	check_file_block:
		Checks a file block against a hash in the hash tree. Returns good if file
		block good. Returns bad if file block bad. Returns io_error if cannot read
		hash tree.
		Precondition: The complete function must return true.
	check_tree_block:
		Check validity of tree block.
		Precondition: Parent tree block must exist or this function will return
			inaccurate results.
	read_block:
		Get block from hash tree. Returns good if suceeded (and block appended to
		buf). Returns io_error if cannot read hash tree.
	root_hash:
		Returns root_hash (top hash of tree) if we have it.
	write_block:
		Add or replace block in hash tree. Returns good if block was written,
		io_error if block could not be written, or bad if the block hash failed.
		The connection_ID is needed for block_request.
		Precondition: Parent hash must have been written.
		Note: This function does not check validity of root_hash (block 0).
	*/
	status check() const;
	status check_file_block(const boost::uint64_t file_block_num, const net::buffer & buf) const;
	status check_tree_block(const boost::uint64_t block_num, const net::buffer & buf) const;
	status read_block(const boost::uint64_t block_num, net::buffer & buf) const;
	boost::optional<std::string> root_hash() const;
	status write_block(const boost::uint64_t block_num, const net::buffer & buf);

	/*
	create:
		Create hash tree. Returns good and sets FI.hash if tree created or already
		exists. Returns bad if tree could never be created. Returns copying if
		file changed size while hashing. Returns io_error if error reading tmp
		file.
		Note: If tree created the tree state will be set to reserved. This must
			be set to complete or tree will be deleted on next program start.
	*/
	static status create(file_info & FI);

private:
	//blob handle for hash tree in database
	const db::blob blob;
};
#endif
