#ifndef H_DOWNLOAD_HASH_TREE
#define H_DOWNLOAD_HASH_TREE

//std
#include <vector>

//custom
#include "download.h"
#include "hash_tree.h"

class download_hash_tree : public download
{
public:
	download_hash_tree(std::string root_hash_in, bool * download_complete_flag_in);

	//documentation for virtual functions in abstract base class
	virtual bool complete();
	virtual unsigned int bytes_expected();
	virtual const std::string & hash();
	virtual const std::string & name();
	virtual unsigned int percent_complete();
	virtual bool request(const int & socket, std::string & request, std::vector<std::pair<char, int> > & expected);
	virtual void response(const int & socket, std::string block);
	virtual void stop();
	virtual const unsigned long & total_size();

private:
	//root hash of the tree downloading
	std::string root_hash;

	/*
	Tells the client that a download is complete, used when download is complete.
	Full description in client.h.
	*/
	bool * download_complete_flag;
};
#endif
