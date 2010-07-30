#include "db_table_prefs.hpp"

//BEGIN static_wrap::static_objects
int db::table::prefs::cache::call_back(int columns, char ** response,
	char ** column_name, std::string & value)
{
	assert(columns == 1);
	assert(std::strcmp(column_name[0], "value") == 0);
	value = response[0];
	return 0;
}

boost::shared_ptr<db::table::prefs::cache::cache_element>
	db::table::prefs::cache::get_cache_element(const std::string & key)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::map<std::string, boost::shared_ptr<cache_element> >::iterator it = Cache.find(key);
	if(it == Cache.end()){
		std::pair<std::map<std::string, boost::shared_ptr<cache_element> >::iterator, bool>
			p = Cache.insert(std::make_pair(key, boost::shared_ptr<cache_element>(new cache_element())));
		return p.first->second;
	}else{
		return it->second;
	}
}

void db::table::prefs::cache::set_query(const std::string query)
{
	DBP_Singleton->get()->query(query);
}
//END static_wrap::static_objects

unsigned db::table::prefs::get_max_download_rate()
{
	return cache::singleton()->get<unsigned>("max_download_rate");
}

unsigned db::table::prefs::get_max_connections()
{
	return cache::singleton()->get<unsigned>("max_connections");
}

unsigned db::table::prefs::get_max_upload_rate()
{
	return cache::singleton()->get<unsigned>("max_upload_rate");
}

std::string db::table::prefs::get_ID()
{
	return cache::singleton()->get<std::string>("ID");
}

std::string db::table::prefs::get_port()
{
	return cache::singleton()->get<std::string>("port");
}

void db::table::prefs::init_cache()
{
	get_max_download_rate();
	get_max_connections();
	get_max_upload_rate();
	get_ID();
	get_port();
}

void db::table::prefs::set_max_download_rate(const unsigned rate)
{
	cache::singleton()->set("max_download_rate", rate);
}

void db::table::prefs::set_max_connections(const unsigned connections)
{
	cache::singleton()->set("max_connections", connections);
}

void db::table::prefs::set_max_upload_rate(const unsigned rate)
{
	cache::singleton()->set("max_upload_rate", rate);
}

void db::table::prefs::set_port(const std::string & port)
{
	cache::singleton()->set("port", port);
}
