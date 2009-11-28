#include "database_init.hpp"

database::init::init()
{
	path::create_required_directories();
	create_all();
}

void database::init::create_all()
{
	database::pool::proxy DB;

	//blacklist
	DB->query("CREATE TABLE IF NOT EXISTS blacklist (IP TEXT UNIQUE)");

	//hash_tree
	DB->query("CREATE TABLE IF NOT EXISTS hash_tree(key INTEGER PRIMARY KEY, hash TEXT, "
		"state INTEGER, tree BLOB)");
	DB->query("CREATE UNIQUE INDEX IF NOT EXISTS hash_tree_hash_index ON hash_tree(hash)");
	DB->query("DELETE FROM hash_tree WHERE state = 0");

	//host
	DB->query("CREATE TABLE IF NOT EXISTS host(hash TEXT, IP TEXT, port TEXT)");
	DB->query("CREATE UNIQUE INDEX IF NOT EXISTS host_index ON host(hash, IP, port)");

	//preferences
	/*
	The INTEGER PRIMARY KEY is used to keep it so there is only ever one row in
	the preferences table. Every time we insert we specify 1 as the key and
	ignore if there is an error. The PRIMARY KEY is unique so if we try to insert
	the row twice we get an error (which we ignore).
	*/
	DB->query("CREATE TABLE IF NOT EXISTS preferences (key INTEGER PRIMARY KEY, "
		"max_connections INTEGER, max_download_rate INTEGER, max_upload_rate INTEGER)");
	std::stringstream ss;
	ss << "INSERT OR IGNORE INTO preferences VALUES(1, " << settings::MAX_CONNECTIONS 
		<< ", " << settings::MAX_DOWNLOAD_RATE << ", " << settings::MAX_UPLOAD_RATE << ")";
	DB->query(ss.str());

	//share
	DB->query("CREATE TABLE IF NOT EXISTS share(hash TEXT, path TEXT, "
		"file_size TEXT, last_write_time TEXT, state INTEGER)");
	DB->query("CREATE INDEX IF NOT EXISTS share_hash_index ON share (hash)");
	DB->query("CREATE INDEX IF NOT EXISTS share_path_index ON share (path)");
	DB->query("CREATE TRIGGER IF NOT EXISTS share_trigger AFTER DELETE ON share "
		"BEGIN DELETE FROM hash_tree WHERE hash = OLD.hash AND NOT EXISTS "
		"(SELECT 1 FROM share WHERE hash = OLD.hash); END;");
}

void database::init::drop_all()
{
	database::pool::proxy DB;
	DB->query("DROP TABLE IF EXISTS blacklist");
	DB->query("DROP TABLE IF EXISTS hash_tree");
	DB->query("DROP TABLE IF EXISTS host");
	DB->query("DROP TABLE IF EXISTS preferences");
	DB->query("DROP TABLE IF EXISTS share");
}
