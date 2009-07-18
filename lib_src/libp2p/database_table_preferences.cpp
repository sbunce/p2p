#include "database_table_preferences.hpp"

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
	unsigned upload_rate;
	DB->query("SELECT max_upload_rate FROM preferences",
		&get_upload_rate_call_back, upload_rate);
	return upload_rate;
}

void database::table::preferences::set_max_download_rate(const unsigned rate,
	database::pool::proxy DB)
{
	std::stringstream ss;
	ss << "UPDATE preferences SET max_download_rate = " << rate;
	DB->query(ss.str());
}

void database::table::preferences::set_max_connections(const unsigned connections,
	database::pool::proxy DB)
{
	std::stringstream ss;
	ss << "UPDATE preferences SET max_connections = " << connections;
	DB->query(ss.str());
}

void database::table::preferences::set_max_upload_rate(const unsigned rate,
	database::pool::proxy DB)
{
	std::stringstream ss;
	ss << "UPDATE preferences SET max_upload_rate = " << rate;
	DB->query(ss.str());
}
