#include "database_table_share.hpp"

void database::table::share::add(const info & Info,
	database::pool::proxy DB)
{
	std::stringstream ss;
	ss << "INSERT INTO share(hash, path, file_size, last_write_time, state) VALUES('"
		<< Info.hash << "', '" << database::escape(Info.path) << "', '" << Info.file_size
		<< "', '" << Info.last_write_time << "', " << Info.file_state << ")";
	DB->query(ss.str());
}

//Note: throws exception if bad data
static void unmarshal_info(int columns, char ** response, char ** column_name,
	database::table::share::info & Info)
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
	Info.file_state = reinterpret_cast<database::table::share::state &>(temp);
}

static int lookup_hash_call_back(int columns, char ** response, char ** column_name,
	boost::shared_ptr<database::table::share::info> & Info)
{
	Info = boost::shared_ptr<database::table::share::info>(
		new database::table::share::info());
	try{
		unmarshal_info(columns, response, column_name, *Info);
	}catch(const std::exception & e){
		LOGGER << e.what();
		Info = boost::shared_ptr<database::table::share::info>();
	}
	return 0;
}

boost::shared_ptr<database::table::share::info> database::table::share::lookup(
	const std::string & hash, database::pool::proxy DB)
{
	std::stringstream ss;
	ss << "SELECT hash, path, file_size, last_write_time, state FROM share WHERE hash = '"
		<< hash << "' LIMIT 1";
	boost::shared_ptr<info> Info;
	DB->query(ss.str(), boost::bind(&lookup_hash_call_back, _1, _2, _3, boost::ref(Info)));
	return Info;
}

//this checks to see if any records with specified hash and size exist
static int remove_call_back_2(int columns_retrieved,
	char ** response, char ** column_name, bool & exists)
{
	exists = true;
	return 0;
}

//this gets the hash and file size that corresponds to a path
static int remove_call_back_1(int columns, char ** response, char ** column_name,
	std::string & path_escaped, database::pool::proxy & DB)
{
	assert(columns == 1);
	assert(std::strcmp(column_name[0], "hash") == 0);

	//delete the record
	std::stringstream ss;
	ss << "DELETE FROM share WHERE path = '" << path_escaped << "'";
	DB->query(ss.str());

	//check if any files with hash still exist
	ss.str(""); ss.clear();
	ss << "SELECT 1 FROM share WHERE hash = '" << response[0] << "' LIMIT 1";
	bool exists = false;
	DB->query(ss.str(), boost::bind(&remove_call_back_2, _1, _2, _3, boost::ref(exists)));
	if(!exists){
		//last file with this hash removed, delete the hash tree
		ss.str(""); ss.clear();
		ss << "DELETE FROM hash WHERE hash = '" << response[0] << "'";
		DB->query(ss.str());
	}
	return 0;
}

void database::table::share::remove(const std::string & path,
	database::pool::proxy DB)
{
	std::string path_escaped = database::escape(path);
	std::stringstream ss;
	ss << "SELECT hash FROM share WHERE path = '" << path_escaped << "' LIMIT 1";
	DB->query(ss.str(), boost::bind(&remove_call_back_1, _1, _2, _3,
		boost::ref(path_escaped), boost::ref(DB)));
}

static int resume_call_back(int columns, char ** response, char ** column_name,
	std::deque<database::table::share::info> & info_container)
{
	try{
		database::table::share::info Info;
		unmarshal_info(columns, response, column_name, Info);
		info_container.push_back(Info);
	}catch(const std::exception & e){
		LOGGER << e.what();
	}
	return 0;
}

std::deque<database::table::share::info> database::table::share::resume(
	database::pool::proxy DB)
{
	std::deque<info> info_container;
	DB->query("SELECT hash, path, file_size, last_write_time, state FROM share",
		boost::bind(&resume_call_back, _1, _2, _3, boost::ref(info_container)));
	return info_container;
}
