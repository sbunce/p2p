#include "database_table_preferences.hpp"

boost::once_flag database::table::preferences::once_flag = BOOST_ONCE_INIT;

static int check_exists_call_back(bool & exists, int columns_retrieved, char ** query_response,
	char ** column_name)
{
	exists = strcmp(query_response[0], "0") != 0;
	return 0;
}

void database::table::preferences::once_func(database::pool::proxy & DB)
{
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

static int get_client_connections_call_back(unsigned & client_connections,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0]);
	std::stringstream ss(response[0]);
	ss >> client_connections;
	return 0;
}

static int get_download_rate_call_back(unsigned & download_rate, int columns_retrieved,
	char ** response, char ** column_name)
{
	assert(response[0]);
	std::stringstream ss(response[0]);
	ss >> download_rate;
	return 0;
}

unsigned database::table::preferences::get_max_download_rate(database::pool::proxy DB)
{
	boost::call_once(once_flag, boost::bind(once_func, DB));
	unsigned download_rate;
	DB->query("SELECT max_download_rate FROM preferences",
		&get_download_rate_call_back, download_rate);
	return download_rate;
}

static int get_max_connections_call_back(unsigned & server_connections,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0]);
	std::stringstream ss(response[0]);
	ss >> server_connections;
	return 0;
}

unsigned database::table::preferences::get_max_connections(database::pool::proxy DB)
{
	boost::call_once(once_flag, boost::bind(once_func, DB));
	unsigned server_connections;
	DB->query("SELECT max_connections FROM preferences",
		&get_max_connections_call_back, server_connections);
	return server_connections;
}

static int get_upload_rate_call_back(unsigned & upload_rate, int columns_retrieved,
	char ** response, char ** column_name)
{
	assert(response[0]);
	std::stringstream ss(response[0]);
	ss >> upload_rate;
	return 0;
}

unsigned database::table::preferences::get_max_upload_rate(database::pool::proxy DB)
{
	boost::call_once(once_flag, boost::bind(once_func, DB));
	unsigned upload_rate;
	DB->query("SELECT max_upload_rate FROM preferences",
		&get_upload_rate_call_back, upload_rate);
	return upload_rate;
}

void database::table::preferences::set_max_download_rate(const unsigned rate,
	database::pool::proxy DB)
{
	boost::call_once(once_flag, boost::bind(once_func, DB));
	std::stringstream ss;
	ss << "UPDATE preferences SET max_download_rate = " << rate;
	DB->query(ss.str());
}

void database::table::preferences::set_max_connections(const unsigned connections,
	database::pool::proxy DB)
{
	boost::call_once(once_flag, boost::bind(once_func, DB));
	std::stringstream ss;
	ss << "UPDATE preferences SET max_connections = " << connections;
	DB->query(ss.str());
}

void database::table::preferences::set_max_upload_rate(const unsigned rate,
	database::pool::proxy DB)
{
	boost::call_once(once_flag, boost::bind(once_func, DB));
	std::stringstream ss;
	ss << "UPDATE preferences SET max_upload_rate = " << rate;
	DB->query(ss.str());
}
