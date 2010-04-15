#include "db_table_blacklist.hpp"

//BEGIN static_wrap
boost::once_flag db::table::blacklist::static_wrap::once_flag = BOOST_ONCE_INIT;

db::table::blacklist::static_wrap::static_wrap()
{
	//thread safe initialization of static data members
	boost::call_once(init, once_flag);
}

atomic_int<int> & db::table::blacklist::static_wrap::blacklist_state()
{
	return _blacklist_state();
}

void db::table::blacklist::static_wrap::init()
{
	_blacklist_state();
}

atomic_int<int> & db::table::blacklist::static_wrap::_blacklist_state()
{
	static atomic_int<int> BS(0);
	return BS;
}
//END static_wrap

void db::table::blacklist::add(const std::string & IP, db::pool::proxy DB)
{
	static_wrap Static_Wrap;
	std::stringstream ss;
	ss << "INSERT OR IGNORE INTO blacklist VALUES ('" << IP << "')";
	DB->query(ss.str());
	++Static_Wrap.blacklist_state();
	if(Static_Wrap.blacklist_state() == 0){
		++Static_Wrap.blacklist_state();
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
	static_wrap Static_Wrap;
	if(last_state_seen == Static_Wrap.blacklist_state()){
		return false;
	}else{
		last_state_seen = Static_Wrap.blacklist_state();
		return true;
	}
}
