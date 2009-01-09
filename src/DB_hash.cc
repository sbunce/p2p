#include "DB_hash.h"

atomic_bool DB_hash::program_start(true);

DB_hash::DB_hash()
: DB(global::DATABASE_PATH)
{
	/*
	The state column holds the values 0, 1, or 2.
	0 - free space
	1 - reserved, but not yet used
	2 - used
	*/
	DB.query("CREATE TABLE IF NOT EXISTS hash (hash TEXT, offset INTEGER, size INTEGER, state INTEGER)");
	DB.query("CREATE INDEX IF NOT EXISTS hash_hash_index ON hash (hash)");
	DB.query("CREATE INDEX IF NOT EXISTS hash_offset_index ON hash (offset)");
	DB.query("CREATE INDEX IF NOT EXISTS hash_size_index ON hash (size)");

	/*
	Unreserve all space upon program start. It's possible that space was reserved,
	but not used, before the program shut down.
	*/
	if(program_start){
		//this only runs upon first instantiation
		DB.query("UPDATE hash SET state = 0 WHERE state = 1");
		program_start = false;
	}
}

void DB_hash::compact()
{

}

void DB_hash::free(const std::string & hash)
{
	std::stringstream ss;
	ss << "UPDATE hash SET state = 0 WHERE hash = '" << hash << "'";
	DB.query(ss.str());
}

/*
First element of pair set to true if space allocated.

If space allocated then second element of pair set to offset of start of space
allocated.
*/
int DB_hash::best_fit_call_back(std::pair<bool, boost::uint64_t> & info,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0] && response[1]);
	info.first = true;

	//size of found free space
	boost::uint64_t found_offset, found_size;

	std::stringstream ss;
	ss << response[0];
	ss >> found_offset;
	ss.str(""); ss.clear();
	ss << response[1];
	ss >> found_size;
	ss.str(""); ss.clear();

	//set state to reserved, resize space if necessary
	ss << "UPDATE hash SET size = " << info.second << ", state = 1 WHERE offset = " << response[0];
	DB.query(ss.str());
	ss.str(""); ss.clear();

	if(found_size != info.second){
		//free space found larger than needed, create entry for unused space
		ss << "INSERT INTO hash VALUES ('', " << found_offset + info.second << ", "
			<< found_size - info.second << ", 0)";
		DB.query(ss.str());
	}

	info.second = found_offset;
	return 0;
}

/*
First element of pair set to true if at least one row exists.

Second element of pair is initially set to needed size. If at least one row
exists then second element set to offset of space allocated.
*/
int DB_hash::create_space_call_back(std::pair<bool, boost::uint64_t> & info,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0] && response[1]);
	info.first = true;

	boost::uint64_t last_offset, last_size, needed_size = info.second;
	std::stringstream ss;
	ss << response[0];
	ss >> last_offset;
	ss.str(""); ss.clear();
	ss << response[1];
	ss >> last_size;
	ss.str(""); ss.clear();

	//start of allocated space is past last highest allocated space
	info.second = last_offset + last_size;
	ss << "INSERT INTO hash VALUES('', " << info.second << ", " << needed_size << ", 1)";
	DB.query(ss.str());
	return 0;
}

boost::uint64_t DB_hash::reserve(const boost::uint64_t tree_size)
{
	//locate best fit (std::pair<true if found, offset>)
	assert(tree_size != 0);
	std::pair<bool, boost::uint64_t> info(false, tree_size);
	DB.query("BEGIN TRANSACTION");
	std::stringstream ss;
	ss << "SELECT offset, size FROM hash WHERE size >= " << tree_size
		<< " AND state = 0 ORDER BY size ASC LIMIT 1";
	DB.query(ss.str(), this, &DB_hash::best_fit_call_back, info);
	ss.str(""); ss.clear();
	if(!info.first){
		//no free space found, create space past last
		DB.query("SELECT offset, size FROM hash ORDER BY offset DESC LIMIT 1",
			this, &DB_hash::create_space_call_back, info);
		ss.str(""); ss.clear();
		if(!info.first){
			//no rows yet exist, create first row
			ss << "INSERT INTO hash VALUES ('', 0, " << tree_size << ", 1)";
			DB.query(ss.str());
			info.second = 0;
		}
	}
	DB.query("END TRANSACTION");
	return info.second;
}

//boost::tuple<set true if found, offset, hash>
int DB_hash::use_call_back(
	boost::tuple<bool, const boost::uint64_t *, const std::string *> & info,
	int columns_retrieved, char ** response, char ** column_name)
{
	//space must first be reserved before it can be used
	assert(response[0]);
	assert(strcmp(response[0], "1") == 0);
	info.get<0>() = true;
	std::stringstream ss;
	ss << "UPDATE hash SET hash = '" << *info.get<2>() << "', state = 2 WHERE offset = "
		<< *info.get<1>();
	DB.query(ss.str());
	return 0;
}

void DB_hash::use(const boost::uint64_t & offset, const std::string & hash)
{
	boost::tuple<bool, const boost::uint64_t *, const std::string *>
		info(false, &offset, &hash);
	std::stringstream ss;
	ss << "SELECT state FROM hash WHERE offset = " << offset;
	DB.query(ss.str(), this, &DB_hash::use_call_back, info);
	assert(info.get<0>());
}
