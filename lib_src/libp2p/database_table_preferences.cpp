#include "database_table_preferences.hpp"

/*
Needed for all the get_* functions which get a unsigned int.
std::pair<true if number retrieved, number>
*/
static int get_unsigned_call_back(
	boost::reference_wrapper<std::pair<bool, unsigned> > number,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0]);
	number.get().first = true;
	std::stringstream ss(response[0]);
	ss >> number.get().second;
	return 0;
}

unsigned database::table::preferences::get_max_download_rate(database::pool::proxy DB)
{
	std::pair<bool, unsigned> download_rate(false, 0);
	DB->query("SELECT max_download_rate FROM preferences", &get_unsigned_call_back,
		boost::ref(download_rate));
	assert(download_rate.first);
	return download_rate.second;
}

unsigned database::table::preferences::get_max_connections(database::pool::proxy DB)
{
	std::pair<bool, unsigned> server_connections(false, 0);
	DB->query("SELECT max_connections FROM preferences", &get_unsigned_call_back,
		boost::ref(server_connections));
	assert(server_connections.first);
	return server_connections.second;
}

unsigned database::table::preferences::get_max_upload_rate(database::pool::proxy DB)
{
	std::pair<bool, unsigned> upload_rate(false, 0);
	DB->query("SELECT max_upload_rate FROM preferences", &get_unsigned_call_back,
		boost::ref(upload_rate));
	assert(upload_rate.first);
	return upload_rate.second;
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
