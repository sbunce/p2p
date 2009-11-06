#include "database_table_preferences.hpp"

//needed for all the get_* functions which get a unsigned int
static int get_unsigned_call_back(int columns, char ** response,
	char ** column_name, unsigned & number)
{
	assert(columns == 1);
	try{
		number = boost::lexical_cast<unsigned>(response[0]);
	}catch(const std::exception & e){
		LOGGER << e.what();
		exit(1);
	}
	return 0;
}

unsigned database::table::preferences::get_max_download_rate(database::pool::proxy DB)
{
	unsigned download_rate;
	DB->query("SELECT max_download_rate FROM preferences",
		boost::bind(&get_unsigned_call_back, _1, _2, _3, boost::ref(download_rate)));
	return download_rate;
}

unsigned database::table::preferences::get_max_connections(database::pool::proxy DB)
{
	unsigned server_connections;
	DB->query("SELECT max_connections FROM preferences",
		boost::bind(&get_unsigned_call_back, _1, _2, _3, boost::ref(server_connections)));
	return server_connections;
}

unsigned database::table::preferences::get_max_upload_rate(database::pool::proxy DB)
{
	unsigned upload_rate;
	DB->query("SELECT max_upload_rate FROM preferences",
		boost::bind(&get_unsigned_call_back, _1, _2, _3, boost::ref(upload_rate)));
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
