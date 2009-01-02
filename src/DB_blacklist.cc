#include "DB_blacklist.h"

DB_blacklist::DB_blacklist()
:
	DB(global::DATABASE_PATH),
	blacklist_state(0)
{
	DB.query("CREATE TABLE IF NOT EXISTS blacklist (IP TEXT UNIQUE)");
	DB.query("CREATE INDEX IF NOT EXISTS blacklist_index ON blacklist (IP)");
}

void DB_blacklist::add(const std::string & IP)
{
	std::ostringstream query;
	query << "INSERT INTO blacklist VALUES ('" << IP << "')";
	DB.query(query.str());
	++blacklist_state;
}

static int is_blacklisted_call_back(bool & found, int columns, char ** response, char ** column_name)
{
	found = true;
	return 0;
}

bool DB_blacklist::is_blacklisted(const std::string & IP)
{
	bool found = false;
	std::ostringstream query;
	query << "SELECT 1 FROM blacklist WHERE IP = '" << IP << "'";
	DB.query(query.str(), &is_blacklisted_call_back, found);
	return found;
}

bool DB_blacklist::modified(int & last_state_seen)
{
	if(last_state_seen == blacklist_state){
		//blacklist has not been updated
		return false;
	}else{
		//blacklist updated
		last_state_seen = blacklist_state;
		return true;
	}
}
