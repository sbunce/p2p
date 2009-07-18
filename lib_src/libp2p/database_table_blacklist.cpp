#include "database_table_blacklist.hpp"

boost::once_flag database::table::blacklist::once_flag = BOOST_ONCE_INIT;
atomic_int<int> database::table::blacklist::blacklist_state(0);

void database::table::blacklist::once_func(database::pool::proxy & DB)
{
	DB->query("CREATE TABLE IF NOT EXISTS blacklist (IP TEXT UNIQUE)");
}

void database::table::blacklist::add(const std::string & IP, database::pool::proxy DB)
{
	boost::call_once(once_flag, boost::bind(once_func, DB));
	std::stringstream ss;
	ss << "INSERT INTO blacklist VALUES ('" << IP << "')";
	DB->query(ss.str());
	++blacklist_state;
}

void database::table::blacklist::clear(database::pool::proxy DB)
{
	boost::call_once(once_flag, boost::bind(once_func, DB));
	DB->query("DELETE FROM blacklist");
}

static int is_blacklisted_call_back(bool & found, int columns, char ** response,
	char ** column_name)
{
	found = true;
	return 0;
}

bool database::table::blacklist::is_blacklisted(const std::string & IP,
	database::pool::proxy DB)
{
	boost::call_once(once_flag, boost::bind(once_func, DB));
	bool found = false;
	std::stringstream ss;
	ss << "SELECT 1 FROM blacklist WHERE IP = '" << IP << "'";
	DB->query(ss.str(), &is_blacklisted_call_back, found);
	return found;
}

bool database::table::blacklist::modified(int & last_state_seen)
{
	if(last_state_seen == blacklist_state){
		return false;
	}else{
		last_state_seen = blacklist_state;
		return true;
	}
}
