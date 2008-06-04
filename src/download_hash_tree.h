#ifndef H_DOWNLOAD_HASH_TREE
#define H_DOWNLOAD_HASH_TREE

//custom
#include "download.h"
#include "hash_tree.h"
#include "request_gen.h"

//std
#include <vector>

class download_hash_tree : public download
{
public:
	download_hash_tree(
		const std::string & root_hash_in,
		const unsigned long & file_size,
		const std::string & file_name,
		volatile bool * download_complete_flag_in
	);

	//documentation for virtual functions in abstract base class
	virtual bool complete();
	virtual const std::string & hash();
	virtual unsigned int max_response_size();
	virtual const std::string & name();
	virtual unsigned int percent_complete();
	virtual bool request(const int & socket, std::string & request, std::vector<std::pair<char, int> > & expected);
	virtual void response(const int & socket, std::string block);
	virtual void stop();
	virtual const uint64_t & total_size();

private:
	//set in ctor
	std::string root_hash;         //root hash of the tree downloading
	uint64_t hash_tree_count; //number of hashes in the tree
	uint64_t hash_tree_size;  //size of the hash tree (bytes)
	std::string hash_name;         //the name of this hash

	//used for percent complete calculation
	uint64_t latest_hash_received;

	/*
	Tells the client that a download is complete, used when download is complete.
	Full description in client.h.
	*/
	volatile bool * download_complete_flag;

	request_gen Request_Gen;
	hash_tree Hash_Tree;
};
#endif
