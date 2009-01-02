#include "DB_share.h"

DB_share::DB_share()
: DB(global::DATABASE_PATH)
{
	DB.query("CREATE TABLE IF NOT EXISTS share (hash TEXT, size TEXT, path TEXT)");
	DB.query("CREATE INDEX IF NOT EXISTS share_path_index ON share (path)");
	DB.query("CREATE INDEX IF NOT EXISTS share_hash_index ON share (hash)");
}

static int add_entry_call_back(bool & entry_exists, int columns_retrieved, char ** query_response, char ** column_name)
{
	entry_exists = true;
	return 0;
}

void DB_share::add_entry(const std::string & hash, const boost::uint64_t & size, const std::string & path)
{
	//determine if entry already exists for file
	bool entry_exists = false;
	char * path_sqlite = sqlite3_mprintf("%q", path.c_str());
	std::ostringstream query;
	query << "SELECT 1 FROM share WHERE path = '" << path_sqlite << "'";
	DB.query(query.str(), &add_entry_call_back, entry_exists);
	sqlite3_free(path_sqlite);

	if(!entry_exists){
		//entry doesn't exist, add one
		char * path_sqlite = sqlite3_mprintf("%q", path.c_str());
		std::ostringstream query;
		query << "INSERT INTO share (hash, size, path) VALUES ('" << hash << "', '" << size << "', '" << path_sqlite << "')";
		DB.query(query.str());
		sqlite3_free(path_sqlite);
	}
}

void DB_share::delete_hash(const std::string & hash, const std::string & path)
{
	std::ostringstream query;
	query << "DELETE FROM share WHERE hash = '" << hash << "' AND path = '" << path << "'";
	DB.query(query.str());
}

static int lookup_hash_0_call_back(bool & entry_exists, int columns_retrieved,
	char ** response, char ** column_name)
{
	assert(response[0]);
	entry_exists = strcmp(response[0], "0") != 0;
	return 0;
}

bool DB_share::lookup_hash(const std::string & hash)
{
	bool entry_exists = false;
	std::ostringstream query;
	query << "SELECT count(1) FROM share WHERE hash = '" << hash << "' LIMIT 1";
	DB.query(query.str(), &lookup_hash_0_call_back, entry_exists);
	return entry_exists;
}

//std::pair<true if found, file path>
static int lookup_hash_1_call_back(std::pair<bool, std::string *> & info,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0]);
	info.first = true;
	*info.second = response[0];
	return 0;
}

bool DB_share::lookup_hash(const std::string & hash, std::string & path)
{
	std::pair<bool, std::string *> info(false, &path);
	std::ostringstream query;
	query << "SELECT path FROM share WHERE hash = '" << hash << "' LIMIT 1";
	DB.query(query.str(), &lookup_hash_1_call_back, info);
	return info.first;
}

//std::pair<true if found, file_size>
static int lookup_hash_2_call_back(std::pair<bool, boost::uint64_t *> & info,
	int columns_retrieved, char ** response, char ** column_name)
{
	info.first = true;
	std::stringstream ss(response[0]);
	ss >> *info.second;
	return 0;
}

bool DB_share::lookup_hash(const std::string & hash, boost::uint64_t & file_size)
{
	std::pair<bool, boost::uint64_t *> info(false, &file_size);
	std::ostringstream query;
	query << "SELECT size FROM share WHERE hash = '" << hash << "' LIMIT 1";
	DB.query(query.str(), &lookup_hash_2_call_back, info);
	return info.first;
}

static int lookup_hash_3_call_back(boost::tuple<bool, boost::uint64_t *, std::string *> & info,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0] && response[1]);
	info.get<0>() = true;
	std::stringstream ss(response[0]);
	ss >> *info.get<1>();
	*info.get<2>() = response[1];
	return 0;
}

bool DB_share::lookup_hash(const std::string & hash, std::string & path, boost::uint64_t & file_size)
{
	boost::tuple<bool, boost::uint64_t *, std::string *> info(false, &file_size, &path);
	std::ostringstream query;
	query << "SELECT size, path FROM share WHERE hash = '" << hash << "' LIMIT 1";
	DB.query(query.str(), &lookup_hash_3_call_back, info);
	return info.get<0>();
}

static int lookup_path_call_back(boost::tuple<bool, std::string *, boost::uint64_t *> & info,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0] && response[1]);
	info.get<0>() = true;
	*info.get<1>() = response[0];
	std::stringstream ss(response[1]);
	ss >> *info.get<2>();
	return 0;
}

bool DB_share::lookup_path(const std::string & path, std::string & hash, boost::uint64_t & size)
{
	boost::tuple<bool, std::string *, boost::uint64_t *> info(false, &hash, &size);
	char * query = sqlite3_mprintf("SELECT hash, size FROM share WHERE path = '%q' LIMIT 1", path.c_str());
	DB.query(query, &lookup_path_call_back, info);
	sqlite3_free(query);
	return info.get<0>();
}

int DB_share::remove_missing_call_back(std::set<std::string> & missing_hashes,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0] && response[1]);
	std::fstream fin(response[1], std::ios::in);
	if(fin.is_open()){
		missing_hashes.erase(response[0]);
	}else{
		std::ostringstream query;
		query << "DELETE FROM share WHERE hash = '" << response[0] << "' AND path = '" << response[1] << "'";
		DB.query(query.str());
		missing_hashes.insert(response[0]);
	}
	return 0;
}

void DB_share::remove_missing(const std::string & share_directory)
{
	std::set<std::string> missing_hashes;
	DB.query("SELECT hash, path FROM share", this, &DB_share::remove_missing_call_back, missing_hashes);

	//remove hash trees with no corresponding files
	std::set<std::string>::iterator iter_cur, iter_end;
	iter_cur = missing_hashes.begin();
	iter_end = missing_hashes.end();
	while(iter_cur != iter_end){
		std::string orphan_hash = global::HASH_DIRECTORY + *iter_cur;
		std::remove(orphan_hash.c_str());
		++iter_cur;
	}
	missing_hashes.clear();
}
