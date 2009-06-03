#ifndef H_SLOT_HASH_TREE
#define H_SLOT_HASH_TREE

//custom
#include "database.hpp"
#include "hash_tree.hpp"
#include "settings.hpp"
#include "slot.hpp"

//std
#include <string>

class slot_hash_tree : public slot
{
public:
	slot_hash_tree(
		std::string * IP_in,
		const std::string & root_hash_in,
		const boost::uint64_t & file_size_in
	);

	/*
	Documentation in slot.h
	*/
	virtual void send_block(const std::string & request, std::string & send);
	virtual void info(std::vector<upload_info> & UI);

private:
	//percent complete uploading (0-100)
	unsigned percent;
	boost::uint64_t highest_block_seen;

	/*
	udpate_percent - calculates percent complete
	                 latest_block: the most recent block requested
	*/
	void update_percent(const boost::uint64_t & latest_block);

	hash_tree::tree_info Tree_Info;
	database::table::blacklist DB_Blacklist;
	database::table::share DB_Share;
	hash_tree Hash_Tree;
};
#endif
