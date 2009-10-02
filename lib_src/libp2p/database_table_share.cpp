#include "database_table_share.hpp"

//BEGIN database::table::share::file_info
database::table::share::file_info::file_info()
{

}

database::table::share::file_info::file_info(
	const ::file_info & FI,
	const state State_in
):
	hash(FI.hash),
	path(FI.path),
	file_size(FI.file_size),
	last_write_time(FI.last_write_time),
	State(State_in)
{

}
//END database::table::share::file_info

void database::table::share::add_entry(const file_info & FI,
	database::pool::proxy DB)
{
	char * path_sqlite = sqlite3_mprintf("%q", FI.path.c_str());
	std::stringstream ss;
	ss << "INSERT INTO share VALUES('" << FI.hash << "', '"
		<< path_sqlite << "', '" << FI.file_size << "', '" << FI.last_write_time
		<< "', " << FI.State << ")";
	sqlite3_free(path_sqlite);
	DB->query(ss.str());
}

//this checks to see if any records with specified hash and size exist
static int delete_entry_call_back_chain_2(boost::reference_wrapper<bool> exists,
	int columns_retrieved, char ** response, char ** column_name)
{
	exists.get() = true;
	return 0;
}

//this gets the hash and file size that corresponds to a path
static int delete_entry_call_back_chain_1(
	boost::reference_wrapper<std::pair<char *, database::pool::proxy &> > info,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(columns_retrieved == 1);

	//delete the record
	std::stringstream ss;
	ss << "DELETE FROM share WHERE path = '" << info.get().first << "'";
	info.get().second->query(ss.str());

	//check if any files with hash still exist
	ss.str(""); ss.clear();
	ss << "SELECT 1 FROM share WHERE hash = '" << response[0] << "' LIMIT 1";
	bool exists = false;
	info.get().second->query(ss.str(), &delete_entry_call_back_chain_2,
		boost::ref(exists));
	if(!exists){
		//last file with this hash removed, delete the hash tree
		database::table::hash::delete_tree(response[0], info.get().second);
	}
	return 0;
}

void database::table::share::delete_entry(const std::string & path,
	database::pool::proxy DB)
{
	char * sqlite3_path = sqlite3_mprintf("%q", path.c_str());
	std::pair<char *, database::pool::proxy &> info(sqlite3_path, DB);
	std::stringstream ss;
	ss << "SELECT hash FROM share WHERE path = '" << sqlite3_path << "' LIMIT 1";
	DB->query(ss.str(), &delete_entry_call_back_chain_1, boost::ref(info));
	sqlite3_free(sqlite3_path);
}

static int lookup_hash_call_back(
	boost::reference_wrapper<boost::shared_ptr<database::table::share::file_info> > FI,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(columns_retrieved == 3);
	FI.get() = boost::shared_ptr<database::table::share::file_info>(
		new database::table::share::file_info());
	try{
		FI.get()->file_size = boost::lexical_cast<boost::uint64_t>(response[0]);
		FI.get()->path = response[1];
		int state = boost::lexical_cast<int>(response[2]);
		FI.get()->State = reinterpret_cast<database::table::share::state &>(state);
	}catch(const std::exception & e){
		LOGGER << e.what();
		FI.get() = boost::shared_ptr<database::table::share::file_info>();
	}
	return 0;
}

boost::shared_ptr<database::table::share::file_info> database::table::share::lookup_hash(
	const std::string & hash, database::pool::proxy DB)
{
	boost::shared_ptr<file_info> FI;
	std::stringstream ss;
	ss << "SELECT file_size, path, state FROM share WHERE hash = '" << hash << "' LIMIT 1";
	DB->query(ss.str(), &lookup_hash_call_back, boost::ref(FI));
	if(FI){
		FI->hash = hash;
	}
	return FI;
}

static int lookup_path_call_back(
	boost::reference_wrapper<boost::shared_ptr<database::table::share::file_info> > FI,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(columns_retrieved == 3);
	FI.get() = boost::shared_ptr<database::table::share::file_info>(
		new database::table::share::file_info());
	try{
		FI.get()->hash = response[0];
		FI.get()->file_size = boost::lexical_cast<boost::uint64_t>(response[1]);
		int state = boost::lexical_cast<int>(response[2]);
		FI.get()->State = reinterpret_cast<database::table::share::state &>(state);
	}catch(const std::exception & e){
		LOGGER << e.what();
		FI.get() = boost::shared_ptr<database::table::share::file_info>();
	}
	return 0;
}

boost::shared_ptr<database::table::share::file_info> database::table::share::lookup_path(
	const std::string & path, database::pool::proxy DB)
{
	char * path_sqlite = sqlite3_mprintf("%q", path.c_str());
	std::stringstream ss;
	ss << "SELECT hash, file_size, state FROM share WHERE path = '" << path_sqlite
		<< "' LIMIT 1";
	sqlite3_free(path_sqlite);
	boost::shared_ptr<file_info> FI;
	DB->query(ss.str(), &lookup_path_call_back, boost::ref(FI));
	if(FI){
		FI->path = path;
	}
	return FI;
}
