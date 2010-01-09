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
	DB->query("CREATE TABLE IF NOT EXISTS blacklist(IP TEXT)");
	DB->query("CREATE UNIQUE INDEX IF NOT EXISTS blacklist_index ON blacklist(IP)");

	//hash
	DB->query("CREATE TABLE IF NOT EXISTS hash(key INTEGER PRIMARY KEY, hash TEXT, "
		"state INTEGER, tree BLOB)");
	DB->query("CREATE UNIQUE INDEX IF NOT EXISTS hash_hash_index ON hash(hash)");
	DB->query("DELETE FROM hash WHERE state = 0");

	//peer
	DB->query("CREATE TABLE IF NOT EXISTS peer(ID TEXT, IP TEXT, port TEXT)");
	DB->query("CREATE UNIQUE INDEX IF NOT EXISTS peer_index ON peer(ID, IP, port)");
	DB->query("CREATE INDEX IF NOT EXISTS peer_ID_index ON peer(ID)");

	//prefs
	DB->query("CREATE TABLE IF NOT EXISTS prefs(key TEXT, value TEXT)");
	DB->query("CREATE UNIQUE INDEX IF NOT EXISTS prefs_key_index ON prefs(key)");
	//set default if key doesn't exist
	std::stringstream ss;
	ss << "INSERT OR IGNORE INTO prefs VALUES('max_connections', '"
		<< settings::MAX_CONNECTIONS << "')";
	DB->query(ss.str());
	ss.str(""); ss.clear();
	ss << "INSERT OR IGNORE INTO prefs VALUES('max_download_rate', '"
		<< settings::MAX_DOWNLOAD_RATE << "')";
	DB->query(ss.str());
	ss.str(""); ss.clear();
	ss << "INSERT OR IGNORE INTO prefs VALUES('max_upload_rate', '"
		<< settings::MAX_UPLOAD_RATE << "')";
	DB->query(ss.str());
	//generate our peer_ID
	unsigned char buf[SHA1::bin_size];
	portable_urandom(buf, SHA1::bin_size, NULL);
	ss.str(""); ss.clear();
	ss << "INSERT OR IGNORE INTO prefs VALUES('peer_ID', '"
		<< convert::bin_to_hex(reinterpret_cast<const char *>(buf), SHA1::bin_size)
		<< "')";
	DB->query(ss.str());
	//generate random port between 1024 - 65535
	union num_byte{
		unsigned num;
		unsigned char byte[sizeof(unsigned)];
	} NB;
	portable_urandom(NB.byte, sizeof(unsigned), NULL);
	ss.str(""); ss.clear();
	ss << "INSERT OR IGNORE INTO prefs VALUES('port', '"
		<< NB.num % 64512 + 1024 << "')";
	DB->query(ss.str());

	//share
	DB->query("CREATE TABLE IF NOT EXISTS share(hash TEXT, path TEXT, "
		"file_size TEXT, last_write_time TEXT, state INTEGER)");
	DB->query("CREATE INDEX IF NOT EXISTS share_hash_index ON share (hash)");
	DB->query("CREATE INDEX IF NOT EXISTS share_path_index ON share (path)");
	DB->query("CREATE TRIGGER IF NOT EXISTS share_trigger AFTER DELETE ON share "
		"BEGIN DELETE FROM hash WHERE hash = OLD.hash AND NOT EXISTS "
		"(SELECT 1 FROM share WHERE hash = OLD.hash); END");

	//source
	DB->query("CREATE TABLE IF NOT EXISTS source(ID TEXT, hash TEXT)");
	DB->query("CREATE UNIQUE INDEX IF NOT EXISTS source_index ON source(ID, hash)");
	DB->query("CREATE INDEX IF NOT EXISTS source_ID_index ON source(ID)");
	DB->query("CREATE INDEX IF NOT EXISTS source_hash_index ON source(hash)");
}

void database::init::drop_all()
{
	database::pool::proxy DB;
	DB->query("DROP TABLE IF EXISTS blacklist");
	DB->query("DROP TABLE IF EXISTS hash");
	DB->query("DROP TABLE IF EXISTS host");
	DB->query("DROP TABLE IF EXISTS peer");
	DB->query("DROP TABLE IF EXISTS prefs");
	DB->query("DROP TABLE IF EXISTS share");
	DB->query("DROP TABLE IF EXISTS source");
}
