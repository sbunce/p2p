#include "database_table_prefs.hpp"

//call back for all get_* functions which get unsigned int
static int get_unsigned_call_back(int columns, char ** response,
	char ** column_name, unsigned & number)
{
	assert(columns == 1);
	assert(std::strcmp(column_name[0], "value") == 0);
	try{
		number = boost::lexical_cast<unsigned>(response[0]);
	}catch(const std::exception & e){
		LOGGER << e.what();
		exit(1);
	}
	return 0;
}

//call back for all get_* functions which get std::string
static int get_string_call_back(int columns, char ** response,
	char ** column_name, std::string & str)
{
	assert(columns == 1);
	assert(std::strcmp(column_name[0], "value") == 0);
	str = response[0];
	return 0;
}

unsigned database::table::prefs::get_max_download_rate(database::pool::proxy DB)
{
	unsigned download_rate;
	DB->query("SELECT value FROM prefs WHERE key = 'max_download_rate'",
		boost::bind(&get_unsigned_call_back, _1, _2, _3, boost::ref(download_rate)));
	return download_rate;
}

unsigned database::table::prefs::get_max_connections(database::pool::proxy DB)
{
	unsigned server_connections;
	DB->query("SELECT value FROM prefs WHERE key = 'max_connections'",
		boost::bind(&get_unsigned_call_back, _1, _2, _3, boost::ref(server_connections)));
	return server_connections;
}

unsigned database::table::prefs::get_max_upload_rate(database::pool::proxy DB)
{
	unsigned upload_rate;
	DB->query("SELECT value FROM prefs WHERE key = 'max_upload_rate'",
		boost::bind(&get_unsigned_call_back, _1, _2, _3, boost::ref(upload_rate)));
	return upload_rate;
}

std::string database::table::prefs::get_peer_ID(database::pool::proxy DB)
{
	std::string peer_ID;
	DB->query("SELECT value FROM prefs WHERE key = 'peer_ID'",
		boost::bind(&get_string_call_back, _1, _2, _3, boost::ref(peer_ID)));
	assert(peer_ID.size() == SHA1::hex_size);
	return peer_ID;
}

std::string database::table::prefs::get_port(database::pool::proxy DB)
{
	std::string port;
	DB->query("SELECT value FROM prefs WHERE key = 'port'",
		boost::bind(&get_string_call_back, _1, _2, _3, boost::ref(port)));
	assert(port.size() <= 5 && std::atoi(port.c_str()) > 0
		&& std::atoi(port.c_str()) < 65536);
	return port;
}

void database::table::prefs::set_max_download_rate(const unsigned rate,
	database::pool::proxy DB)
{
	std::stringstream ss;
	ss << "UPDATE prefs SET value = '" << rate << "' WHERE key = 'max_download_rate'";
	DB->query(ss.str());
}

void database::table::prefs::set_max_connections(const unsigned connections,
	database::pool::proxy DB)
{
	std::stringstream ss;
	ss << "UPDATE prefs SET value = '" << connections << "' WHERE key = 'max_connections'";
	DB->query(ss.str());
}

void database::table::prefs::set_max_upload_rate(const unsigned rate,
	database::pool::proxy DB)
{
	std::stringstream ss;
	ss << "UPDATE prefs SET value = '" << rate << "' WHERE key = 'max_upload_rate'";
	DB->query(ss.str());
}

void database::table::prefs::set_port(const std::string & port,
		database::pool::proxy DB)
{
	assert(port.size() <= 5 && std::atoi(port.c_str()) > 0
		&& std::atoi(port.c_str()) < 65536);
	std::stringstream ss;
	ss << "UPDATE prefs SET value = '" << port << "' WHERE key = 'port'";
	DB->query(ss.str());
}
