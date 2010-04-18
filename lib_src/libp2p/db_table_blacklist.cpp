#include "db_table_blacklist.hpp"

//BEGIN static_wrap
boost::once_flag db::table::blacklist::static_wrap::once_flag = BOOST_ONCE_INIT;

db::table::blacklist::static_wrap::static_objects &
	db::table::blacklist::static_wrap::get()
{
	boost::call_once(once_flag, _get);
	return _get();
}

db::table::blacklist::static_wrap::static_objects &
	db::table::blacklist::static_wrap::_get()
{
	static static_objects SO;
	return SO;
}
//END static_wrap

void db::table::blacklist::add(const std::string & IP, db::pool::proxy DB)
{
	std::stringstream ss;
	ss << "INSERT OR IGNORE INTO blacklist VALUES ('" << IP << "')";
	DB->query(ss.str());
	++static_wrap::get().blacklist_state;
	if(static_wrap::get().blacklist_state == 0){
		++static_wrap::get().blacklist_state;
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
	if(last_state_seen == static_wrap::get().blacklist_state){
		return false;
	}else{
		last_state_seen = static_wrap::get().blacklist_state;
		return true;
	}
}
