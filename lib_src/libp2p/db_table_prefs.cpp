#include "db_table_prefs.hpp"

//BEGIN static_wrap
boost::once_flag db::table::prefs::static_wrap::once_flag = BOOST_ONCE_INIT;

db::table::prefs::static_wrap::static_objects & db::table::prefs::static_wrap::get()
{
	boost::call_once(once_flag, _get);
	return _get();
}

db::table::prefs::static_wrap::static_objects & db::table::prefs::static_wrap::_get()
{
	static static_objects SO;
	return SO;
}
//END static_wrap

//BEGIN static_wrap::static_objects
int db::table::prefs::static_wrap::static_objects::call_back(int columns,
	char ** response, char ** column_name, std::string & value)
{
	assert(columns == 1);
	assert(std::strcmp(column_name[0], "value") == 0);
	value = response[0];
	return 0;
}

boost::shared_ptr<db::table::prefs::static_wrap::static_objects::cache_element>
	db::table::prefs::static_wrap::static_objects::get_cache_element(
	const std::string & key)
{
	boost::mutex::scoped_lock lock(Cache_mutex);
	std::map<std::string, boost::shared_ptr<cache_element> >::iterator it = Cache.find(key);
	if(it == Cache.end()){
		std::pair<std::map<std::string, boost::shared_ptr<cache_element> >::iterator, bool>
			p = Cache.insert(std::make_pair(key, boost::shared_ptr<cache_element>(new cache_element())));
		return p.first->second;
	}else{
		return it->second;
	}
}
//END static_wrap::static_objects

unsigned db::table::prefs::get_max_download_rate(db::pool::proxy DB)
{
	return static_wrap::get().get<unsigned>("max_download_rate", DB);
}

unsigned db::table::prefs::get_max_connections(db::pool::proxy DB)
{
	return static_wrap::get().get<unsigned>("max_connections", DB);
}

unsigned db::table::prefs::get_max_upload_rate(db::pool::proxy DB)
{
	return static_wrap::get().get<unsigned>("max_upload_rate", DB);
}

std::string db::table::prefs::get_ID(db::pool::proxy DB)
{
	return static_wrap::get().get<std::string>("ID", DB);
}

std::string db::table::prefs::get_port(db::pool::proxy DB)
{
	return static_wrap::get().get<std::string>("port", DB);
}

void db::table::prefs::init_cache()
{
	db::pool::proxy DB;
	get_max_download_rate(DB);
	get_max_connections(DB);
	get_max_upload_rate(DB);
	get_ID(DB);
	get_port(DB);
}

void db::table::prefs::set_max_download_rate(const unsigned rate,
	db::pool::proxy DB)
{
	static_wrap::get().set("max_download_rate", rate, DB);
}

void db::table::prefs::set_max_connections(const unsigned connections,
	db::pool::proxy DB)
{
	static_wrap::get().set("max_connections", connections, DB);
}

void db::table::prefs::set_max_upload_rate(const unsigned rate,
	db::pool::proxy DB)
{
	static_wrap::get().set("max_upload_rate", rate, DB);
}

void db::table::prefs::set_port(const std::string & port,
	db::pool::proxy DB)
{
	static_wrap::get().set("port", port, DB);
}
