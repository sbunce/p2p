#include "database_init.hpp"

void database::init::blacklist(database::connection & DB)
{
	DB.query("CREATE TABLE IF NOT EXISTS blacklist (IP TEXT UNIQUE)");
	DB.query("CREATE INDEX IF NOT EXISTS blacklist_index ON blacklist (IP)");
}

void database::init::hash(database::connection & DB)
{
	DB.query("CREATE TABLE IF NOT EXISTS hash(key INTEGER PRIMARY KEY, hash TEXT, state INTEGER, size INTEGER, tree BLOB)");
	DB.query("CREATE INDEX IF NOT EXISTS hash_hash_index ON hash(hash)");
	DB.query("CREATE INDEX IF NOT EXISTS hash_state_index ON hash(state)");
	DB.query("CREATE INDEX IF NOT EXISTS hash_size_index ON hash(size)");
	DB.query("DELETE FROM hash WHERE state = 0");
}

static int check_exists_call_back(bool & exists, int columns_retrieved, char ** query_response,
	char ** column_name)
{
	exists = strcmp(query_response[0], "0") != 0;
	return 0;
}

void database::init::preferences(database::connection & DB)
{
	DB.query("CREATE TABLE IF NOT EXISTS preferences (max_connections INTEGER, \
		max_download_rate INTEGER, max_upload_rate INTEGER)");
	bool exists = false;
	DB.query("SELECT count(1) FROM preferences", &check_exists_call_back, exists);
	if(!exists){
		//set defaults if no preferences yet exist
		std::stringstream ss;
		ss << "INSERT INTO preferences VALUES(" << settings::MAX_CONNECTIONS 
		<< ", " << settings::MAX_DOWNLOAD_RATE << ", " << settings::MAX_UPLOAD_RATE << ")";
		DB.query(ss.str());
	}
}

void database::init::prime(database::connection & DB)
{
	DB.query("CREATE TABLE IF NOT EXISTS prime (number TEXT)");
}

void database::init::run(const std::string & path)
{
	database::connection DB(path);
	blacklist(DB);
	hash(DB);
	preferences(DB);
	prime(DB);
	search(DB);
	share(DB);
}

void database::init::search(database::connection & DB)
{
	bool exists = false;
	DB.query("SELECT 1 FROM sqlite_master WHERE name = 'search'", &check_exists_call_back, exists);
	if(!exists){
		/*
		There is no "CREATE VIRTUAL TABLE IF NOT EXISTS" in sqlite3 (although it may
		be added eventually). This is needed to avoid trying to create the table
		twice and causing errors.
		*/
		DB.query("CREATE VIRTUAL TABLE search USING FTS3 (hash TEXT, name TEXT, size TEXT, server TEXT)");
	}
}

void database::init::share(database::connection & DB)
{
	DB.query("CREATE TABLE IF NOT EXISTS share (hash TEXT, size TEXT, path TEXT)");
	DB.query("CREATE INDEX IF NOT EXISTS share_hash_index ON share (hash)");
	DB.query("CREATE INDEX IF NOT EXISTS share_path_index ON share (path)");
}
