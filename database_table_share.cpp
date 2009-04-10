#include "database_table_share.hpp"

database::table::share::share():
	DB(path::database())
{

}

void database::table::share::add_entry(const std::string & hash,
	const boost::uint64_t & size, const std::string & path)
{
	add_entry(hash, size, path, DB);
}

void database::table::share::add_entry(const std::string & hash,
	const boost::uint64_t & size, const std::string & path,
	database::connection & DB)
{
	char * path_sqlite = sqlite3_mprintf("%q", path.c_str());
	std::stringstream ss;
	ss << "INSERT INTO share(hash, size, path) VALUES('" << hash << "', '" << size << "', '" << path_sqlite << "')";
	sqlite3_free(path_sqlite);
	DB.query(ss.str());
}

void database::table::share::delete_entry(const std::string & path)
{
	delete_entry(path, DB);
}

void database::table::share::delete_entry(const std::string & path,
	database::connection & DB)
{
	char * sqlite3_path = sqlite3_mprintf("%Q", path.c_str());
	DB.query("BEGIN TRANSACTION");
	std::string hash;
	boost::uint64_t file_size;
	if(lookup_path(path, hash, file_size, DB)){
		database::table::hash::delete_tree(hash, hash_tree::tree_info::file_size_to_tree_size(file_size), DB);
	}
	std::stringstream ss;
	ss << "DELETE FROM share WHERE hash = '" << hash << "' AND path = " << sqlite3_path;
	sqlite3_free(sqlite3_path);
	DB.query(ss.str());
	DB.query("END TRANSACTION");
}

static int lookup_hash_0_call_back(bool & entry_exists, int columns_retrieved,
	char ** response, char ** column_name)
{
	assert(response[0]);
	entry_exists = strcmp(response[0], "0") != 0;
	return 0;
}

bool database::table::share::lookup_hash(const std::string & hash)
{
	return lookup_hash(hash, DB);
}

bool database::table::share::lookup_hash(const std::string & hash,
	database::connection & DB)
{
	bool entry_exists = false;
	std::stringstream ss;
	ss << "SELECT count(1) FROM share WHERE hash = '" << hash << "' LIMIT 1";
	DB.query(ss.str(), &lookup_hash_0_call_back, entry_exists);
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

bool database::table::share::lookup_hash(const std::string & hash, std::string & path)
{
	return lookup_hash(hash, path, DB);
}

bool database::table::share::lookup_hash(const std::string & hash,
	std::string & path, database::connection & DB)
{
	std::pair<bool, std::string *> info(false, &path);
	std::stringstream ss;
	ss << "SELECT path FROM share WHERE hash = '" << hash << "' LIMIT 1";
	DB.query(ss.str(), &lookup_hash_1_call_back, info);
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

bool database::table::share::lookup_hash(const std::string & hash,
	boost::uint64_t & file_size)
{
	return lookup_hash(hash, file_size, DB);
}

bool database::table::share::lookup_hash(const std::string & hash,
	boost::uint64_t & file_size, database::connection & DB)
{
	std::pair<bool, boost::uint64_t *> info(false, &file_size);
	std::stringstream ss;
	ss << "SELECT size FROM share WHERE hash = '" << hash << "' LIMIT 1";
	DB.query(ss.str(), &lookup_hash_2_call_back, info);
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

bool database::table::share::lookup_hash(const std::string & hash,
	std::string & path, boost::uint64_t & file_size)
{
	return lookup_hash(hash, path, file_size, DB);
}

bool database::table::share::lookup_hash(const std::string & hash,
	std::string & path, boost::uint64_t & file_size, database::connection & DB)
{
	boost::tuple<bool, boost::uint64_t *, std::string *> info(false, &file_size, &path);
	std::stringstream ss;
	ss << "SELECT size, path FROM share WHERE hash = '" << hash << "' LIMIT 1";
	DB.query(ss.str(), &lookup_hash_3_call_back, info);
	return info.get<0>();
}

static int lookup_path_call_back(boost::tuple<bool, std::string *, boost::uint64_t *> & info,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0] && response[1]);
	info.get<0>() = true;
	*info.get<1>() = response[0];
	std::stringstream ss;
	ss << response[1];
	ss >> *info.get<2>();
	return 0;
}

bool database::table::share::lookup_path(const std::string & path,
	std::string & hash, boost::uint64_t & file_size)
{
	return lookup_path(path, hash, file_size, DB);
}

bool database::table::share::lookup_path(const std::string & path, std::string & hash,
	boost::uint64_t & file_size, database::connection & DB)
{
	boost::tuple<bool, std::string *, boost::uint64_t *> info(false, &hash, &file_size);
	char * query = sqlite3_mprintf("SELECT hash, size FROM share WHERE path = '%q' LIMIT 1", path.c_str());
	DB.query(query, &lookup_path_call_back, info);
	sqlite3_free(query);
	return info.get<0>();
}

static int unique_key_call_back(bool & unique, int columns_retrieved,
	char ** response, char ** column_name)
{
	assert(response[0]);
	unique = strcmp(response[0], "1") == 0;
	return 0;
}

bool database::table::share::unique_hash(const std::string & hash,
	database::connection & DB)
{
	bool unique = false;
	std::stringstream ss;
	ss << "SELECT count(1) FROM share WHERE hash = '" << hash << "'";
	DB.query(ss.str(), &unique_key_call_back, unique);
	return unique;
}
