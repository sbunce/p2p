#include "DB_server_preferences.h"

boost::mutex DB_server_preferences::Mutex;

//server_preferences should only have one row, this checks to see if it exists
static int check_exists(bool & exists, int columns_retrieved, char ** response, char ** column_name)
{
	exists = strcmp(response[0], "0") != 0;
	return 0;
}

DB_server_preferences::DB_server_preferences()
: DB(global::DATABASE_PATH)
{
	boost::mutex::scoped_lock lock(Mutex);
	DB.query("CREATE TABLE IF NOT EXISTS server_preferences (share_directory TEXT, speed_limit TEXT, max_connections TEXT)");
	bool exists;
	DB.query("SELECT count(1) FROM server_preferences", &check_exists, exists);
	if(!exists){
		std::stringstream query;
		query << "INSERT INTO server_preferences VALUES('" << global::SHARE_DIRECTORY
			<< "', '" << global::UP_SPEED << "', '" << global::MAX_CONNECTIONS << "')";
		DB.query(query.str());
	}
}


static int get_share_directory_call_back(std::string & share_directory,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0]);
	share_directory.assign(response[0]);
	return 0;
}

std::string DB_server_preferences::get_share_directory()
{
	std::string share_directory;
	DB.query("SELECT share_directory FROM server_preferences",
		&get_share_directory_call_back, share_directory);
	return share_directory;
}

void DB_server_preferences::set_share_directory(const std::string & share_directory)
{
	std::ostringstream query;
	query << "UPDATE server_preferences SET share_directory = '" << share_directory << "'";
	DB.query(query.str());
}

static int get_speed_limit_call_back(unsigned & speed_limit, int columns_retrieved,
	char ** response, char ** column_name)
{
	assert(response[0]);
	std::stringstream ss(response[0]);
	ss >> speed_limit;
	return 0;
}

unsigned DB_server_preferences::get_speed_limit()
{
	unsigned speed_limit;
	DB.query("SELECT speed_limit FROM server_preferences", &get_speed_limit_call_back, speed_limit);
	return speed_limit;
}

void DB_server_preferences::set_speed_limit(const unsigned & speed_limit)
{
	std::ostringstream query;
	query << "UPDATE server_preferences SET speed_limit = '" << speed_limit << "'";
	DB.query(query.str());
}

static int get_max_connections_call_back(unsigned & max_connections, int columns_retrieved,
	char ** response, char ** column_name)
{
	assert(response[0]);
	std::stringstream ss(response[0]);
	ss >> max_connections;
	return 0;
}

unsigned DB_server_preferences::get_max_connections()
{
	unsigned max_connections;
	DB.query("SELECT max_connections FROM server_preferences", &get_max_connections_call_back, max_connections);
	return max_connections;
}

void DB_server_preferences::set_max_connections(const unsigned & max_connections)
{
	std::ostringstream query;
	query << "UPDATE server_preferences SET max_connections = '" << max_connections << "'";
	DB.query(query.str());
}
