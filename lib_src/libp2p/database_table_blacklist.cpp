#include "database_table_blacklist.hpp"

atomic_int<int> database::table::blacklist::blacklist_state(0);

void database::table::blacklist::add(const std::string & IP, database::pool::proxy DB)
{
	std::stringstream ss;
	ss << "INSERT OR IGNORE INTO blacklist VALUES ('" << IP << "')";
	DB->query(ss.str());
	++blacklist_state;
}

void database::table::blacklist::clear(database::pool::proxy DB)
{
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
