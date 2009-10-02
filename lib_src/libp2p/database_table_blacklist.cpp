#include "database_table_blacklist.hpp"

boost::once_flag database::table::blacklist::once_flag = BOOST_ONCE_INIT;

void database::table::blacklist::add(const std::string & IP, database::pool::proxy DB)
{
	boost::call_once(init, once_flag);
	std::stringstream ss;
	ss << "INSERT OR IGNORE INTO blacklist VALUES ('" << IP << "')";
	DB->query(ss.str());
	++blacklist_state();
}

atomic_int<int> & database::table::blacklist::blacklist_state()
{
	static atomic_int<int> * b = new atomic_int<int>(0);
	return *b;
}

void database::table::blacklist::init()
{
	blacklist_state();
}

static int is_blacklisted_call_back(boost::reference_wrapper<bool> found,
	int columns, char ** response, char ** column_name)
{
	found.get() = true;
	return 0;
}

bool database::table::blacklist::is_blacklisted(const std::string & IP,
	database::pool::proxy DB)
{
	boost::call_once(init, once_flag);
	bool found = false;
	std::stringstream ss;
	ss << "SELECT 1 FROM blacklist WHERE IP = '" << IP << "'";
	DB->query(ss.str(), &is_blacklisted_call_back, boost::ref(found));
	return found;
}

bool database::table::blacklist::modified(int & last_state_seen)
{
	boost::call_once(init, once_flag);
	if(last_state_seen == blacklist_state()){
		return false;
	}else{
		last_state_seen = blacklist_state();
		return true;
	}
}
