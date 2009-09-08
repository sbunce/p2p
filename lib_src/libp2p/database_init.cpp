#include "database_init.hpp"

static int check_exists_call_back(bool & exists, int columns_retrieved,
	char ** response, char ** column_name)
{
	assert(response[0]);
	exists = strcmp(response[0], "0") != 0;
	return 0;
}

database::init::init()
{
	path::create_required_directories();
	create_all();
}

void database::init::blacklist()
{
	database::pool::proxy DB;
	DB->query("CREATE TABLE IF NOT EXISTS blacklist (IP TEXT UNIQUE)");
}

void database::init::create_all()
{
	blacklist();
	hash();
	host();
	preferences();
	prime();
	share();
}

void database::init::drop_all()
{
	database::pool::proxy DB;
	DB->query("DROP TABLE IF EXISTS blacklist");
	DB->query("DROP TABLE IF EXISTS hash");
	DB->query("DROP TABLE IF EXISTS host");
	DB->query("DROP TABLE IF EXISTS preferences");
	DB->query("DROP TABLE IF EXISTS prime");
	DB->query("DROP TABLE IF EXISTS share");
}

void database::init::hash()
{
	database::pool::proxy DB;
	DB->query("CREATE TABLE IF NOT EXISTS hash(key INTEGER PRIMARY KEY, hash TEXT, tree_size INTEGER, tree BLOB, state INTEGER)");
	DB->query("CREATE INDEX IF NOT EXISTS hash_hash_index ON hash(hash)");
	DB->query("CREATE INDEX IF NOT EXISTS hash_size_index ON hash(tree_size)");
	DB->query("CREATE INDEX IF NOT EXISTS hash_state_index ON hash(state)");
	DB->query("DELETE FROM hash WHERE state = 0");
}

void database::init::host()
{
	database::pool::proxy DB;
	DB->query("CREATE TABLE IF NOT EXISTS host(hash TEXT, host TEXT, port TEXT)");
	DB->query("CREATE INDEX IF NOT EXISTS host_hash_index ON host(hash)");
	DB->query("CREATE UNIQUE INDEX IF NOT EXISTS host_hash_host_index ON host(hash, host, port)");
}

void database::init::preferences()
{
	database::pool::proxy DB;
	DB->query("CREATE TABLE IF NOT EXISTS preferences (max_connections INTEGER, max_download_rate INTEGER, max_upload_rate INTEGER)");
	bool exists = false;
	DB->query("SELECT count(1) FROM preferences", &check_exists_call_back, exists);
	if(!exists){
		//set defaults if no preferences yet exist
		std::stringstream ss;
		ss << "INSERT INTO preferences VALUES(" << settings::MAX_CONNECTIONS 
			<< ", " << settings::MAX_DOWNLOAD_RATE << ", " << settings::MAX_UPLOAD_RATE << ")";
		DB->query(ss.str());
	}
}

void database::init::prime()
{
	database::pool::proxy DB;
	DB->query("CREATE TABLE IF NOT EXISTS prime (number TEXT)");
}

void database::init::share()
{
	database::pool::proxy DB;
	DB->query("CREATE TABLE IF NOT EXISTS share (hash TEXT, file_size TEXT, path TEXT, state INTEGER)");
	DB->query("CREATE INDEX IF NOT EXISTS share_hash_index ON share (hash)");
	DB->query("CREATE INDEX IF NOT EXISTS share_path_index ON share (path)");
}
