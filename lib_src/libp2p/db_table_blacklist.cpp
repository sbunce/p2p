#include "db_table_blacklist.hpp"

boost::once_flag db::table::blacklist::once_flag = BOOST_ONCE_INIT;

void db::table::blacklist::add(const std::string & IP, db::pool::proxy DB)
{
	boost::call_once(init, once_flag);
	std::stringstream ss;
	ss << "INSERT OR IGNORE INTO blacklist VALUES ('" << IP << "')";
	DB->query(ss.str());
	++blacklist_state();
	if(blacklist_state() == 0){
		++blacklist_state();
	}
}

atomic_int<int> & db::table::blacklist::blacklist_state()
{
	static atomic_int<int> b(0);
	return b;
}

void db::table::blacklist::init()
{
	blacklist_state();
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
	boost::call_once(init, once_flag);
	bool found = false;
	std::stringstream ss;
	ss << "SELECT 1 FROM blacklist WHERE IP = '" << IP << "'";
	DB->query(ss.str(), boost::bind(&is_blacklisted_call_back, _1, _2, _3, boost::ref(found)));
	return found;
}

bool db::table::blacklist::modified(int & last_state_seen)
{
	boost::call_once(init, once_flag);
	if(last_state_seen == blacklist_state()){
		return false;
	}else{
		last_state_seen = blacklist_state();
		return true;
	}
}
