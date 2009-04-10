#include "database_table_preferences.hpp"

database::table::preferences::preferences():
	DB(path::database())
{

}

static int get_client_connections_call_back(unsigned & client_connections,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0]);
	std::stringstream ss(response[0]);
	ss >> client_connections;
	return 0;
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
	return get_download_directory(DB);
}

std::string database::table::preferences::get_download_directory(database::connection & DB)
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

unsigned database::table::preferences::get_max_download_rate()
{
	return get_max_download_rate(DB);
}

unsigned database::table::preferences::get_max_download_rate(database::connection & DB)
{
	unsigned download_rate;
	DB.query("SELECT download_rate FROM preferences", &get_download_rate_call_back, download_rate);
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

unsigned database::table::preferences::get_max_connections()
{
	return get_max_connections(DB);
}

unsigned database::table::preferences::get_max_connections(database::connection & DB)
{
	unsigned server_connections;
	DB.query("SELECT max_connections FROM preferences",
		&get_max_connections_call_back, server_connections);
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
	return get_share_directory(DB);
}

std::string database::table::preferences::get_share_directory(database::connection & DB)
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

unsigned database::table::preferences::get_max_upload_rate()
{
	return get_max_upload_rate(DB);
}

unsigned database::table::preferences::get_max_upload_rate(database::connection & DB)
{
	unsigned upload_rate;
	DB.query("SELECT upload_rate FROM preferences",
		&get_upload_rate_call_back, upload_rate);
	return upload_rate;
}

void database::table::preferences::set_download_directory(const std::string & download_directory)
{
	set_download_directory(download_directory, DB);
}

void database::table::preferences::set_download_directory(const std::string & download_directory, database::connection & DB)
{
	std::stringstream ss;
	ss << "UPDATE preferences SET download_directory = '"
		<< download_directory << "'";
	DB.query(ss.str());
}

void database::table::preferences::set_max_download_rate(const unsigned & rate)
{
	set_max_download_rate(rate, DB);
}

void database::table::preferences::set_max_download_rate(const unsigned & rate, database::connection & DB)
{
	std::stringstream ss;
	ss << "UPDATE preferences SET download_rate = " << rate;
	DB.query(ss.str());
}

void database::table::preferences::set_max_connections(const unsigned & connections)
{
	set_max_connections(connections, DB);
}

void database::table::preferences::set_max_connections(const unsigned & connections, database::connection & DB)
{
	std::stringstream ss;
	ss << "UPDATE preferences SET max_connections = " << connections;
	DB.query(ss.str());
}

void database::table::preferences::set_share_directory(const std::string & share_directory)
{
	set_share_directory(share_directory, DB);
}

void database::table::preferences::set_share_directory(const std::string & share_directory, database::connection & DB)
{
	std::stringstream ss;
	ss << "UPDATE preferences SET share_directory = '"
		<< share_directory << "'";
	DB.query(ss.str());
}

void database::table::preferences::set_max_upload_rate(const unsigned & rate)
{
	set_max_upload_rate(rate, DB);
}

void database::table::preferences::set_max_upload_rate(const unsigned & rate, database::connection & DB)
{
	std::stringstream ss;
	ss << "UPDATE preferences SET upload_rate = " << rate;
	DB.query(ss.str());
}
