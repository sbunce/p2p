#include "database_init.hpp"

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
	DB->query("CREATE UNIQUE INDEX IF NOT EXISTS hash_hash_index ON hash(hash)");
	DB->query("CREATE INDEX IF NOT EXISTS hash_size_index ON hash(tree_size)");
	DB->query("CREATE INDEX IF NOT EXISTS hash_state_index ON hash(state)");
	DB->query("DELETE FROM hash WHERE state = 0");
}

void database::init::host()
{
	database::pool::proxy DB;
	DB->query("CREATE TABLE IF NOT EXISTS host(hash TEXT, host TEXT, port TEXT)");
	DB->query("CREATE INDEX IF NOT EXISTS host_hash_index ON host(hash)");
	DB->query("CREATE UNIQUE INDEX IF NOT EXISTS host_index ON host(hash, host, port)");
}

void database::init::preferences()
{
	/*
	The INTEGER PRIMARY KEY is used to keep it so there is only ever one row in
	the preferences table. Every time we insert we specify 1 as the key and
	ignore if there is an error. The PRIMARY KEY is unique so if we try to insert
	the row twice we get an error (which we ignore).
	*/
	database::pool::proxy DB;
	DB->query("CREATE TABLE IF NOT EXISTS preferences (key INTEGER PRIMARY KEY, max_connections INTEGER, max_download_rate INTEGER, max_upload_rate INTEGER)");
	std::stringstream ss;
	ss << "INSERT OR IGNORE INTO preferences VALUES(1, " << settings::MAX_CONNECTIONS 
		<< ", " << settings::MAX_DOWNLOAD_RATE << ", " << settings::MAX_UPLOAD_RATE << ")";
	DB->query(ss.str());
}

void database::init::prime()
{
	database::pool::proxy DB;
	DB->query("CREATE TABLE IF NOT EXISTS prime (number TEXT)");
}

void database::init::share()
{
	database::pool::proxy DB;
	DB->query("CREATE TABLE IF NOT EXISTS share (hash TEXT, path TEXT, file_size TEXT, last_write_time TEXT, state INTEGER)");
	DB->query("CREATE INDEX IF NOT EXISTS share_hash_index ON share (hash)");
	DB->query("CREATE INDEX IF NOT EXISTS share_path_index ON share (path)");
}
