#include "db_table_share.hpp"

void db::table::share::add(const info & Info,
	db::pool::proxy DB)
{
	std::stringstream ss;
	ss << "INSERT INTO share(hash, path, file_size, last_write_time, state) VALUES('"
		<< Info.hash << "', '" << db::escape(Info.path) << "', '" << Info.file_size
		<< "', '" << Info.last_write_time << "', " << Info.file_state << ")";
	DB->query(ss.str());
}

//Note: throws exception if bad data
static void unmarshal_info(int columns, char ** response, char ** column_name,
	db::table::share::info & Info)
{
	assert(columns == 5);
	assert(std::strcmp(column_name[0], "hash") == 0);
	assert(std::strcmp(column_name[1], "path") == 0);
	assert(std::strcmp(column_name[2], "file_size") == 0);
	assert(std::strcmp(column_name[3], "last_write_time") == 0);
	assert(std::strcmp(column_name[4], "state") == 0);
	Info.hash = response[0];
	Info.path = response[1];
	Info.file_size = boost::lexical_cast<boost::uint64_t>(response[2]);
	Info.last_write_time = boost::lexical_cast<std::time_t>(response[3]);
	int temp = boost::lexical_cast<int>(response[4]);
	Info.file_state = reinterpret_cast<db::table::share::state &>(temp);
}

static int find_hash_call_back(int columns, char ** response, char ** column_name,
	boost::shared_ptr<db::table::share::info> & Info)
{
	Info = boost::shared_ptr<db::table::share::info>(
		new db::table::share::info());
	try{
		unmarshal_info(columns, response, column_name, *Info);
	}catch(const std::exception & e){
		LOG << e.what();
		Info = boost::shared_ptr<db::table::share::info>();
	}
	return 0;
}

boost::shared_ptr<db::table::share::info> db::table::share::find(
	const std::string & hash, db::pool::proxy DB)
{
	std::stringstream ss;
	ss << "SELECT hash, path, file_size, last_write_time, state FROM share WHERE hash = '"
		<< hash << "' LIMIT 1";
	boost::shared_ptr<info> Info;
	DB->query(ss.str(), boost::bind(&find_hash_call_back, _1, _2, _3, boost::ref(Info)));
	return Info;
}

void db::table::share::remove(const std::string & path,
	db::pool::proxy DB)
{
	std::string path_escaped = db::escape(path);
	std::stringstream ss;
	ss << "DELETE FROM share WHERE path = '" << path_escaped << "'";
	DB->query(ss.str());
}

static int resume_call_back(int columns, char ** response, char ** column_name,
	std::deque<db::table::share::info> & info_container)
{
	if(boost::this_thread::interruption_requested()){
		//resume query can take a long time, give a chance to end early
		return -1;
	}
	try{
		db::table::share::info Info;
		unmarshal_info(columns, response, column_name, Info);
		info_container.push_back(Info);
	}catch(const std::exception & e){
		LOG << e.what();
	}
	return 0;
}

std::deque<db::table::share::info> db::table::share::resume(
	db::pool::proxy DB)
{
	std::deque<info> info_container;
	DB->query("SELECT hash, path, file_size, last_write_time, state FROM share",
		boost::bind(&resume_call_back, _1, _2, _3, boost::ref(info_container)));
	return info_container;
}

void db::table::share::set_state(const std::string & hash,
	const state file_state, db::pool::proxy DB)
{
	std::stringstream ss;
	ss << "UPDATE share SET state = " << file_state << " WHERE hash = '" << hash << "'";
	DB->query(ss.str());
}

void db::table::share::update_file_size(const std::string & path,
	const boost::uint64_t file_size, db::pool::proxy DB)
{
	std::stringstream ss;
	ss << "UPDATE OR IGNORE share SET file_size = '" << file_size
		<< "' WHERE path = '" << db::escape(path) << "'";
	DB->query(ss.str()); 
}
