#include "database_table_blacklist.hpp"

//BEGIN static
boost::once_flag database::table::blacklist::once_flag = BOOST_ONCE_INIT;
atomic_int<int> * database::table::blacklist::_blacklist_state;

void database::table::blacklist::init()
{
	_blacklist_state = new atomic_int<int>(0);
}

atomic_int<int> & database::table::blacklist::blacklist_state()
{
	boost::call_once(init, once_flag);
	return *_blacklist_state;
}
//END static

void database::table::blacklist::add(const std::string & IP, database::connection & DB)
{
	std::stringstream ss;
	ss << "INSERT INTO blacklist VALUES ('" << IP << "')";
	DB.query(ss.str());
	++blacklist_state();
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
