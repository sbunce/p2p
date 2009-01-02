#include "DB_client_preferences.h"

boost::mutex DB_client_preferences::Mutex;

//client_preferences should only have one row, this checks to see if it exists
int check_exists(bool & exists, int columns_retrieved, char ** query_response,
	char ** column_name)
{
	exists = strcmp(query_response[0], "0") != 0;
	return 0;
}

DB_client_preferences::DB_client_preferences()
:
	DB(global::DATABASE_PATH)
{
	boost::mutex::scoped_lock lock(Mutex);
	DB.query("CREATE TABLE IF NOT EXISTS client_preferences (download_directory TEXT, speed_limit TEXT, max_connections TEXT)");
	bool exists;
	DB.query("SELECT count(1) FROM client_preferences", &check_exists, exists);
	if(!exists){
		std::stringstream query;
		query << "INSERT INTO client_preferences VALUES('" << global::DOWNLOAD_DIRECTORY
			<< "', '" << global::DOWN_SPEED << "', '" << global::MAX_CONNECTIONS << "')";
		DB.query(query.str());
	}
}

static int get_download_directory_call_back(std::string & download_directory,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0]);
	download_directory.assign(response[0]);
	return 0;
}

std::string DB_client_preferences::get_download_directory()
{
	std::string download_directory;
	DB.query("SELECT download_directory FROM client_preferences",
		&get_download_directory_call_back, download_directory);
	return download_directory;
}

void DB_client_preferences::set_download_directory(
	const std::string & download_directory)
{
	std::ostringstream query;
	query << "UPDATE client_preferences SET download_directory = '"
		<< download_directory << "'";
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

unsigned DB_client_preferences::get_speed_limit()
{
	unsigned speed_limit;
	DB.query("SELECT speed_limit FROM client_preferences",
		&get_speed_limit_call_back, speed_limit);
	return speed_limit;
}

void DB_client_preferences::set_speed_limit(const unsigned & speed_limit)
{
	std::ostringstream query;
	query << "UPDATE client_preferences SET speed_limit = '" << speed_limit << "'";
	DB.query(query.str());
}

static int get_max_connections_call_back(unsigned & max_connections,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0]);
	std::stringstream ss(response[0]);
	ss >> max_connections;
	return 0;
}

unsigned DB_client_preferences::get_max_connections()
{
	unsigned max_connections;
	DB.query("SELECT max_connections FROM client_preferences",
		&get_max_connections_call_back, max_connections);
	return max_connections;
}

void DB_client_preferences::set_max_connections(const unsigned & max_connections)
{
	std::ostringstream query;
	query << "UPDATE client_preferences SET max_connections = '"
		<< max_connections << "'";
	DB.query(query.str());
}
