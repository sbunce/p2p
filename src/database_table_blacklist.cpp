#include "database_table_blacklist.hpp"

atomic_int<int> & database::table::blacklist::blacklist_state()
{
	static atomic_int<int> * bs = new atomic_int<int>(0);
	return *bs;
}

void database::table::blacklist::add(const std::string & IP)
{
	add(IP, DB);
}

void database::table::blacklist::add(const std::string & IP, database::connection & DB)
{
	std::stringstream ss;
	ss << "INSERT INTO blacklist VALUES ('" << IP << "')";
	DB.query(ss.str());
	++blacklist_state();
}

bool database::table::blacklist::is_blacklisted(const std::string & IP)
{
	return is_blacklisted(IP, DB);
}

static int is_blacklisted_call_back(bool & found, int columns, char ** response, char ** column_name)
{
	found = true;
	return 0;
}

bool database::table::blacklist::is_blacklisted(const std::string & IP, database::connection & DB)
{
	bool found = false;
	std::stringstream ss;
	ss << "SELECT 1 FROM blacklist WHERE IP = '" << IP << "'";
	DB.query(ss.str(), &is_blacklisted_call_back, found);
	return found;
}

bool database::table::blacklist::modified(int & last_state_seen)
{
	if(last_state_seen == blacklist_state()){
		//blacklist has not been updated
		return false;
	}else{
		//blacklist updated
		last_state_seen = blacklist_state();
		return true;
	}
}
