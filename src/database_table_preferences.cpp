#include "database_table_preferences.hpp"

boost::mutex database::table::preferences::Mutex;

static int check_exists(bool & exists, int columns_retrieved, char ** query_response,
	char ** column_name)
{
	exists = strcmp(query_response[0], "0") != 0;
	return 0;
}

database::table::preferences::preferences()
{
	boost::mutex::scoped_lock lock(Mutex);
	DB.query("CREATE TABLE IF NOT EXISTS preferences (client_connections INTEGER, \
		download_directory TEXT, download_rate INTEGER, server_connections INTEGER, \
		share_directory TEXT, upload_rate INTEGER)");
	bool exists;
	DB.query("SELECT count(1) FROM preferences", &check_exists, exists);
	if(!exists){
		//set defaults if no preferences yet exist
		std::stringstream ss;
		ss << "INSERT INTO preferences VALUES(" << global::CLIENT_CONNECTIONS
			<< ", '" << global::DOWNLOAD_DIRECTORY << "', " << global::DOWNLOAD_RATE
			<< ", " << global::SERVER_CONNECTIONS << ", '" << global::SHARE_DIRECTORY
			<< "', " << global::UPLOAD_RATE << ")";
		DB.query(ss.str());
	}
}

static int get_client_connections_call_back(unsigned & client_connections,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0]);
	std::stringstream ss(response[0]);
	ss >> client_connections;
	return 0;
}

unsigned database::table::preferences::get_client_connections()
{
	unsigned client_connections;
	DB.query("SELECT client_connections FROM preferences",
		&get_client_connections_call_back, client_connections);
	return client_connections;
}


static int get_download_directory_call_back(std::string & download_directory,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0]);
	download_directory.assign(response[0]);
	return 0;
}

std::string database::table::preferences::get_download_directory()
{
	std::string download_directory;
	DB.query("SELECT download_directory FROM preferences",
		&get_download_directory_call_back, download_directory);
	return download_directory;
}

static int get_download_rate_call_back(unsigned & download_rate, int columns_retrieved,
	char ** response, char ** column_name)
{
	assert(response[0]);
	std::stringstream ss(response[0]);
	ss >> download_rate;
	return 0;
}

unsigned database::table::preferences::get_download_rate()
{
	unsigned download_rate;
	DB.query("SELECT download_rate FROM preferences", &get_download_rate_call_back, download_rate);
	return download_rate;
}

static int get_server_connections_call_back(unsigned & server_connections,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0]);
	std::stringstream ss(response[0]);
	ss >> server_connections;
	return 0;
}

unsigned database::table::preferences::get_server_connections()
{
	unsigned server_connections;
	DB.query("SELECT server_connections FROM preferences",
		&get_server_connections_call_back, server_connections);
	return server_connections;
}

static int get_share_directory_call_back(std::string & share_directory,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0]);
	share_directory.assign(response[0]);
	return 0;
}

std::string database::table::preferences::get_share_directory()
{
	std::string share_directory;
	DB.query("SELECT share_directory FROM preferences",
		&get_share_directory_call_back, share_directory);
	return share_directory;
}

static int get_upload_rate_call_back(unsigned & upload_rate, int columns_retrieved,
	char ** response, char ** column_name)
{
	assert(response[0]);
	std::stringstream ss(response[0]);
	ss >> upload_rate;
	return 0;
}

unsigned database::table::preferences::get_upload_rate()
{
	unsigned upload_rate;
	DB.query("SELECT upload_rate FROM preferences",
		&get_upload_rate_call_back, upload_rate);
	return upload_rate;
}

void database::table::preferences::set_client_connections(const unsigned & connections)
{
	std::stringstream ss;
	ss << "UPDATE preferences SET client_connections = " << connections;
	DB.query(ss.str());
}

void database::table::preferences::set_download_directory(const std::string & download_directory)
{
	std::stringstream ss;
	ss << "UPDATE preferences SET download_directory = '"
		<< download_directory << "'";
	DB.query(ss.str());
}

void database::table::preferences::set_download_rate(const unsigned & rate)
{
	std::stringstream ss;
	ss << "UPDATE preferences SET download_rate = " << rate;
	DB.query(ss.str());
}

void database::table::preferences::set_server_connections(const unsigned & connections)
{
	std::stringstream ss;
	ss << "UPDATE preferences SET server_connections = " << connections;
	DB.query(ss.str());
}

void database::table::preferences::set_share_directory(const std::string & share_directory)
{
	std::stringstream ss;
	ss << "UPDATE preferences SET share_directory = '"
		<< share_directory << "'";
	DB.query(ss.str());
}

void database::table::preferences::set_upload_rate(const unsigned & rate)
{
	std::stringstream ss;
	ss << "UPDATE preferences SET upload_rate = " << rate;
	DB.query(ss.str());
}
