#include "database_init.hpp"
#include "database_connection.hpp"

static int check_exists_call_back(bool & exists, int columns_retrieved, char ** query_response,
	char ** column_name)
{
	exists = strcmp(query_response[0], "0") != 0;
	return 0;
}

static void setup_blacklist(database::connection & DB)
{
	DB.query("CREATE TABLE IF NOT EXISTS blacklist (IP TEXT UNIQUE)");
	DB.query("CREATE INDEX IF NOT EXISTS blacklist_index ON blacklist (IP)");
}

static void setup_download(database::connection & DB)
{
	DB.query("CREATE TABLE IF NOT EXISTS download (hash TEXT, name TEXT, size TEXT, server TEXT)");
}

static void setup_hash(database::connection & DB)
{
	DB.query("CREATE TABLE IF NOT EXISTS hash(key INTEGER PRIMARY KEY, hash TEXT, state INTEGER, size INTEGER, tree BLOB)");
	DB.query("CREATE INDEX IF NOT EXISTS hash_hash_index ON hash(hash)");
	DB.query("CREATE INDEX IF NOT EXISTS hash_state_index ON hash(state)");
	DB.query("CREATE INDEX IF NOT EXISTS hash_size_index ON hash(size)");
	DB.query("DELETE FROM hash WHERE state = 0");
}

static void setup_preferences(database::connection & DB)
{
	DB.query("CREATE TABLE IF NOT EXISTS preferences (download_directory TEXT, \
		share_directory TEXT, max_connections INTEGER, download_rate INTEGER, \
		upload_rate INTEGER)");
	bool exists = false;
	DB.query("SELECT count(1) FROM preferences", &check_exists_call_back, exists);
	if(!exists){
		//set defaults if no preferences yet exist
		std::stringstream ss;
		ss << "INSERT INTO preferences VALUES('" << global::DOWNLOAD_DIRECTORY
		<< "', '" << global::SHARE_DIRECTORY << "', " << global::MAX_CONNECTIONS 
		<< ", " << global::DOWNLOAD_RATE << ", " << global::UPLOAD_RATE << ")";
		DB.query(ss.str());
	}
}

static void setup_prime(database::connection & DB)
{
	DB.query("CREATE TABLE IF NOT EXISTS prime (key INTEGER PRIMARY KEY, number TEXT)");
}

static void setup_search(database::connection & DB)
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

static void setup_share(database::connection & DB)
{
	DB.query("CREATE TABLE IF NOT EXISTS share (hash TEXT, size TEXT, path TEXT)");
	DB.query("CREATE INDEX IF NOT EXISTS share_hash_index ON share (hash)");
	DB.query("CREATE INDEX IF NOT EXISTS share_path_index ON share (path)");
}

void database::init::run()
{
	//need recursive mutex because instantiating database::connection calls this
	static boost::recursive_mutex Mutex;
	static bool program_start(true);
	boost::recursive_mutex::scoped_lock lock(Mutex);
	if(program_start){
		program_start = false;

		database::connection DB;
		setup_blacklist(DB);
		setup_download(DB);
		setup_hash(DB);
		setup_preferences(DB);
		setup_prime(DB);
		setup_search(DB);
		setup_share(DB);
	}
}
