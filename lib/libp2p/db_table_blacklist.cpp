#include "db_table_blacklist.hpp"

void db::table::blacklist::add(const std::string & IP, db::pool::proxy DB)
{
	std::stringstream ss;
	ss << "INSERT OR IGNORE INTO blacklist VALUES ('" << IP << "')";
	DB->query(ss.str());
	++wrap::singleton()->blacklist_state;
	if(wrap::singleton()->blacklist_state == 0){
		++wrap::singleton()->blacklist_state;
	}
}

static int is_blacklisted_call_back(int columns, char ** response,
	char ** column_name, bool & found)
{
	found = true;
	return 0;
}

bool db::table::blacklist::is_blacklisted(const std::string & IP,
	db::pool::proxy DB)
{
	bool found = false;
	std::stringstream ss;
	ss << "SELECT 1 FROM blacklist WHERE IP = '" << IP << "'";
	DB->query(ss.str(), boost::bind(&is_blacklisted_call_back, _1, _2, _3, boost::ref(found)));
	return found;
}

bool db::table::blacklist::modified(int & last_state_seen)
{
	if(last_state_seen == wrap::singleton()->blacklist_state){
		return false;
	}else{
		last_state_seen = wrap::singleton()->blacklist_state;
		return true;
	}
}
