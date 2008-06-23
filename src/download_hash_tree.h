#ifndef H_DOWNLOAD_HASH_TREE
#define H_DOWNLOAD_HASH_TREE

//custom
#include "convert.h"
#include "download.h"
#include "download_hash_tree_conn.h"
#include "hash_tree.h"
#include "hex.h"
#include "request_gen.h"
#include "sha.h"

//std
#include <vector>

class download_hash_tree : public download
{
public:
	download_hash_tree(
		const std::string & root_hash_in,
		const uint64_t & file_size_in,
		const std::string & file_name_in
	);

	//this information is for the file the hash tree was generated for
	uint64_t file_size;
	std::string file_name;

	/*
	If the hash tree download gets cancelled early by the user this will be set
	to true which will make the completion of the hash tree not trigger a
	download_file to start.
	*/
	bool canceled;

	//documentation for virtual functions in abstract base class
	virtual bool complete();
	virtual const std::string & hash();
	virtual const std::string & name();
	virtual unsigned int percent_complete();
	virtual bool request(const int & socket, std::string & request, std::vector<std::pair<char, int> > & expected);
	virtual void response(const int & socket, std::string block);
	virtual void stop();
	virtual const uint64_t size();

private:
	/*
	After P_CLOSE_SLOT is sent to all servers and there are no pending responses
	from any server this should be set to true.
	*/
	bool download_complete;

	std::string root_hash;     //root hash of the tree downloading
	std::string hash_name;     //the name of this hash
	uint64_t hash_tree_count;  //number of hashes in the tree
	uint64_t hash_block_count; //number of hash blocks ((global::P_BLOCK_SIZE - 1) / sha::HASH_LENGTH)
	uint64_t hashes_per_block; //number of hashes in a hash block

	/*
	When the file has finished downloading this will be set to true to indicate
	that P_CLOSE_SLOT commands need to be sent to all servers.
	*/
	bool close_slots;

	convert<uint64_t> Convert_uint64;
	request_gen Request_Gen;
	hash_tree Hash_Tree;
};
#endif
