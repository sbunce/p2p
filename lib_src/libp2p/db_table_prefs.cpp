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

static int call_back(int columns, char ** response, char ** column_name,
	std::string & value)
{
	assert(columns == 1);
	assert(std::strcmp(column_name[0], "value") == 0);
	value = response[0];
	return 0;
}

static unsigned str_to_unsigned(const std::string & str)
{
	try{
		return boost::lexical_cast<unsigned>(str);
	}catch(const std::exception & e){
		LOG << e.what();
		exit(1);
	}
}

unsigned db::table::prefs::get_max_download_rate(db::pool::proxy DB)
{
	{//BEGIN lock scope
	boost::mutex::scoped_lock lock(static_wrap::get().max_download_rate_mutex);
	if(static_wrap::get().max_download_rate){
		return *static_wrap::get().max_download_rate;
	}
	}//END lock scope

	std::string value;
	DB->query("SELECT value FROM prefs WHERE key = 'max_download_rate'",
		boost::bind(&call_back, _1, _2, _3, boost::ref(value)));

	{//BEGIN lock scope
	boost::mutex::scoped_lock lock(static_wrap::get().max_download_rate_mutex);
	if(static_wrap::get().max_download_rate){
		return *static_wrap::get().max_download_rate;
	}else{
		static_wrap::get().max_download_rate.reset(str_to_unsigned(value));
		return *static_wrap::get().max_download_rate;
	}
	}//END lock scope
}

unsigned db::table::prefs::get_max_connections(db::pool::proxy DB)
{
	{//BEGIN lock scope
	boost::mutex::scoped_lock lock(static_wrap::get().max_connections_mutex);
	if(static_wrap::get().max_connections){
		return *static_wrap::get().max_connections;
	}
	}//END lock scope

	std::string value;
	DB->query("SELECT value FROM prefs WHERE key = 'max_connections'",
		boost::bind(&call_back, _1, _2, _3, boost::ref(value)));

	{//BEGIN lock scope
	boost::mutex::scoped_lock lock(static_wrap::get().max_connections_mutex);
	if(static_wrap::get().max_connections){
		return *static_wrap::get().max_connections;
	}else{
		static_wrap::get().max_connections.reset(str_to_unsigned(value));
		return *static_wrap::get().max_connections;
	}
	}//END lock scope
}

unsigned db::table::prefs::get_max_upload_rate(db::pool::proxy DB)
{
	{//BEGIN lock scope
	boost::mutex::scoped_lock lock(static_wrap::get().max_upload_rate_mutex);
	if(static_wrap::get().max_upload_rate){
		return *static_wrap::get().max_upload_rate;
	}
	}//END lock scope

	std::string value;
	DB->query("SELECT value FROM prefs WHERE key = 'max_upload_rate'",
		boost::bind(&call_back, _1, _2, _3, boost::ref(value)));

	{//BEGIN lock scope
	boost::mutex::scoped_lock lock(static_wrap::get().max_upload_rate_mutex);
	if(static_wrap::get().max_upload_rate){
		return *static_wrap::get().max_upload_rate;
	}else{
		static_wrap::get().max_upload_rate.reset(str_to_unsigned(value));
		return *static_wrap::get().max_upload_rate;
	}
	}//END lock scope
}

std::string db::table::prefs::get_ID(db::pool::proxy DB)
{
	{//BEGIN lock scope
	boost::mutex::scoped_lock lock(static_wrap::get().ID_mutex);
	if(static_wrap::get().ID){
		return *static_wrap::get().ID;
	}
	}//END lock scope

	std::string value;
	DB->query("SELECT value FROM prefs WHERE key = 'ID'",
		boost::bind(&call_back, _1, _2, _3, boost::ref(value)));

	{//BEGIN lock scope
	boost::mutex::scoped_lock lock(static_wrap::get().ID_mutex);
	if(static_wrap::get().ID){
		return *static_wrap::get().ID;
	}else{
		static_wrap::get().ID.reset(value);
		return *static_wrap::get().ID;
	}
	}//END lock scope
}

std::string db::table::prefs::get_port(db::pool::proxy DB)
{
	{//BEGIN lock scope
	boost::mutex::scoped_lock lock(static_wrap::get().port_mutex);
	if(static_wrap::get().port){
		return *static_wrap::get().port;
	}
	}//END lock scope

	std::string value;
	DB->query("SELECT value FROM prefs WHERE key = 'port'",
		boost::bind(&call_back, _1, _2, _3, boost::ref(value)));

	{//BEGIN lock scope
	boost::mutex::scoped_lock lock(static_wrap::get().port_mutex);
	if(static_wrap::get().port){
		return *static_wrap::get().port;
	}else{
		static_wrap::get().port.reset(value);
		return *static_wrap::get().port;
	}
	}//END lock scope
}

/*
The db connection would be tied up if we passed it to the thread_pool
enqueue function. This function allows us to get a db connection only when we're
ready to do the query.
*/
static void set_wrapper(const std::string query)
{
	db::pool::get()->query(query);
}

void db::table::prefs::set_max_download_rate(const unsigned rate,
	db::pool::proxy DB)
{
	{//BEGIN lock scope
	boost::mutex::scoped_lock lock(static_wrap::get().max_download_rate_mutex);
	static_wrap::get().max_download_rate.reset(rate);
	}//END lock scope

	std::stringstream ss;
	ss << "UPDATE prefs SET value = '" << rate << "' WHERE key = 'max_download_rate'";
	static_wrap::get().Thread_Pool.enqueue(boost::bind(&set_wrapper, ss.str()));
}

void db::table::prefs::set_max_connections(const unsigned connections,
	db::pool::proxy DB)
{
	{//BEGIN lock scope
	boost::mutex::scoped_lock lock(static_wrap::get().max_connections_mutex);
	static_wrap::get().max_connections.reset(connections);
	}//END lock scope

	std::stringstream ss;
	ss << "UPDATE prefs SET value = '" << connections << "' WHERE key = 'max_connections'";
	static_wrap::get().Thread_Pool.enqueue(boost::bind(&set_wrapper, ss.str()));
}

void db::table::prefs::set_max_upload_rate(const unsigned rate,
	db::pool::proxy DB)
{
	{//BEGIN lock scope
	boost::mutex::scoped_lock lock(static_wrap::get().max_upload_rate_mutex);
	static_wrap::get().max_upload_rate.reset(rate);
	}//END lock scope

	std::stringstream ss;
	ss << "UPDATE prefs SET value = '" << rate << "' WHERE key = 'max_upload_rate'";
	static_wrap::get().Thread_Pool.enqueue(boost::bind(&set_wrapper, ss.str()));
}

void db::table::prefs::set_port(const std::string & port,
		db::pool::proxy DB)
{
	{//BEGIN lock scope
	boost::mutex::scoped_lock lock(static_wrap::get().port_mutex);
	static_wrap::get().port.reset(port);
	}//END lock scope

	assert(port.size() <= 5 && std::atoi(port.c_str()) > 0
		&& std::atoi(port.c_str()) < 65536);
	std::stringstream ss;
	ss << "UPDATE prefs SET value = '" << port << "' WHERE key = 'port'";
	static_wrap::get().Thread_Pool.enqueue(boost::bind(&set_wrapper, ss.str()));
}

void db::table::prefs::warm_up_cache(db::pool::proxy DB)
{
	get_max_download_rate(DB);
	get_max_connections(DB);
	get_max_upload_rate(DB);
	get_ID(DB);
	get_port(DB);
}
