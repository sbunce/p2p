#include "database_table_share.hpp"

//BEGIN database::table::share::file_info
database::table::share::file_info::file_info()
{

}

database::table::share::file_info::file_info(
	const std::string & hash_in,
	const boost::uint64_t & file_size_in,
	const std::string & path_in,
	const state State_in
):
	hash(hash_in),
	file_size(file_size_in),
	path(path_in),
	State(State_in)
{

}
//END database::table::share::file_info

void database::table::share::add_entry(const file_info & FI,
	database::pool::proxy DB)
{
	char * path_sqlite = sqlite3_mprintf("%Q", FI.path.c_str());
	std::stringstream ss;
	ss << "INSERT INTO share VALUES('" << FI.hash << "', '"
		<< FI.file_size << "', " << path_sqlite << ", " << FI.State << ")";
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
	boost::reference_wrapper<std::pair<char *, database::pool::proxy *> > info,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0]);

	//delete the record
	std::stringstream ss;
	ss << "DELETE FROM share WHERE path = " << info.get().first;
	(*info.get().second)->query(ss.str());

	//check if any records remain with hash
	ss.str(""); ss.clear();
	ss << "SELECT 1 FROM share WHERE hash = '" << response[0] << "' LIMIT 1";
	bool exists = false;
	(*info.get().second)->query(ss.str(), &delete_entry_call_back_chain_2,
		boost::ref(exists));
	if(!exists){
		//last file with this hash removed, delete the hash tree
		database::table::hash::delete_tree(response[0], *info.get().second);
	}
	return 0;
}

void database::table::share::delete_entry(const std::string & path,
	database::pool::proxy DB)
{
	char * sqlite3_path = sqlite3_mprintf("%Q", path.c_str());
	std::pair<char *, database::pool::proxy *> info(sqlite3_path, &DB);
	std::stringstream ss;
	ss << "SELECT hash FROM share WHERE path = " << sqlite3_path << " LIMIT 1";
	DB->query(ss.str(), &delete_entry_call_back_chain_1, boost::ref(info));
	sqlite3_free(sqlite3_path);
}

static int lookup_hash_call_back(
	boost::reference_wrapper<boost::shared_ptr<database::table::share::file_info> > FI,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0] && response[1] && response[2]);
	FI.get() = boost::shared_ptr<database::table::share::file_info>(
		new database::table::share::file_info());

	std::stringstream ss;

	//file_size
	ss << response[0];
	ss >> FI.get()->file_size;

	//path
	FI.get()->path = response[1];

	//state
	ss.str(""); ss.clear();
	ss << response[2];
	int temp;
	ss >> temp;
	FI.get()->State = reinterpret_cast<database::table::share::state &>(temp);

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
	assert(response[0] && response[1]);
	FI.get() = boost::shared_ptr<database::table::share::file_info>(
		new database::table::share::file_info());

	std::stringstream ss;

	//hash
	FI.get()->hash = response[0];

	//file size
	ss << response[1];
	ss >> FI.get()->file_size;

	//state
	ss << response[2];
	int temp;
	ss >> temp;
	FI.get()->State = reinterpret_cast<database::table::share::state &>(temp);

	return 0;
}

boost::shared_ptr<database::table::share::file_info> database::table::share::lookup_path(
	const std::string & path, database::pool::proxy DB)
{
	boost::shared_ptr<file_info> FI;
	char * query = sqlite3_mprintf(
		"SELECT hash, file_size, state FROM share WHERE path = %Q LIMIT 1", path.c_str());
	DB->query(query, &lookup_path_call_back, boost::ref(FI));
	sqlite3_free(query);
	if(FI){
		FI->path = path;
	}
	return FI;
}

static int resume_call_back(
	boost::reference_wrapper<std::vector<database::table::share::file_info> > Resume,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0] && response[1] && response[2]);
	database::table::share::file_info FI;
	FI.hash = response[0];
	std::stringstream ss;
	ss << response[1];
	ss >> FI.file_size;
	FI.path = response[2];
	Resume.get().push_back(FI);
	return 0;
}

void database::table::share::resume(
	std::vector<database::table::share::file_info> & Resume,
	database::pool::proxy DB)
{
	std::stringstream ss;
	ss << "SELECT hash, file_size, path FROM share WHERE state = " << downloading;
	DB->query(ss.str(), &resume_call_back, boost::ref(Resume));
	for(std::vector<file_info>::iterator iter_cur = Resume.begin(),
		iter_end = Resume.end(); iter_cur != iter_end; ++iter_cur)
	{
		iter_cur->State = downloading;
	}
}
