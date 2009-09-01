#include "database_table_share.hpp"

void database::table::share::add_entry(const file_info & FI,
	database::pool::proxy DB)
{
	char * path_sqlite = sqlite3_mprintf("%Q", FI.path.c_str());
	std::stringstream ss;
	ss << "INSERT INTO share(hash, file_size, path, state) VALUES('" << FI.hash << "', '"
		<< FI.file_size << "', " << path_sqlite << ", " << FI.State << ")";
	sqlite3_free(path_sqlite);
	DB->query(ss.str());
}

//this checks to see if any records with specified hash and size exist
static int delete_entry_call_back_chain_2(bool & exists, int columns_retrieved,
	char ** response, char ** column_name)
{
	exists = true;
	return 0;
}

//this gets the hash and file size that corresponds to a path
static int delete_entry_call_back_chain_1(std::pair<char *, database::pool::proxy *> & info,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0]);

	//delete the record
	std::stringstream ss;
	ss << "DELETE FROM share WHERE path = " << info.first;
	(*info.second)->query(ss.str());

	//check if any records remain with hash
	ss.str(""); ss.clear();
	ss << "SELECT 1 FROM share WHERE hash = '" << response[0] << "' LIMIT 1";
	bool exists = false;
	(*info.second)->query(ss.str(), &delete_entry_call_back_chain_2, exists);
	if(!exists){
		//last file with this hash removed, delete the hash tree
		database::table::hash::delete_tree(response[0], *info.second);
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
	DB->query(ss.str(), &delete_entry_call_back_chain_1, info);
	sqlite3_free(sqlite3_path);
}

static int lookup_hash_call_back(boost::shared_ptr<database::table::share::file_info> & FI,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0] && response[1] && response[2]);
	FI = boost::shared_ptr<database::table::share::file_info>(
		new database::table::share::file_info());

	std::stringstream ss;

	//file_size
	ss << response[0];
	ss >> FI->file_size;

	//path
	FI->path = response[1];

	//state
	ss.str(""); ss.clear();
	ss << response[2];
	int temp;
	ss >> temp;
	FI->State = reinterpret_cast<database::table::share::state &>(temp);

	return 0;
}

boost::shared_ptr<database::table::share::file_info> database::table::share::lookup_hash(
	const std::string & hash, database::pool::proxy DB)
{
	boost::shared_ptr<file_info> FI;
	std::stringstream ss;
	ss << "SELECT file_size, path, state FROM share WHERE hash = '" << hash << "' LIMIT 1";
	DB->query(ss.str(), &lookup_hash_call_back, FI);
	if(FI){
		FI->hash = hash;
	}
	return FI;
}

static int lookup_path_call_back(
	boost::shared_ptr<database::table::share::file_info> & FI,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0] && response[1]);
	FI = boost::shared_ptr<database::table::share::file_info>(
		new database::table::share::file_info());

	std::stringstream ss;

	//hash
	FI->hash = response[0];

	//file size
	ss << response[1];
	ss >> FI->file_size;

	//state
	ss << response[2];
	int temp;
	ss >> temp;
	FI->State = reinterpret_cast<database::table::share::state &>(temp);

	return 0;
}

boost::shared_ptr<database::table::share::file_info> database::table::share::lookup_path(
	const std::string & path, database::pool::proxy DB)
{
	boost::shared_ptr<file_info> FI;
	char * query = sqlite3_mprintf(
		"SELECT hash, file_size, state FROM share WHERE path = %Q LIMIT 1", path.c_str());
	DB->query(query, &lookup_path_call_back, FI);
	sqlite3_free(query);
	if(FI){
		FI->path = path;
	}
	return FI;
}

static int resume_call_back(
	std::vector<database::table::share::file_info> & Resume,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0] && response[1] && response[2]);
	database::table::share::file_info FI;

	std::stringstream ss;

	//hash
	FI.hash = response[0];

	//file size
	ss << response[1];
	ss >> FI.file_size;

	//path
	FI.path = response[2];

	Resume.push_back(FI);

	return 0;
}

void database::table::share::resume(std::vector<database::table::share::file_info> & Resume,
	database::pool::proxy DB)
{
	std::stringstream ss;
	ss << "SELECT hash, file_size, path FROM share WHERE state = " << DOWNLOADING;
	DB->query(ss.str(), &resume_call_back, Resume);

	for(std::vector<file_info>::iterator iter_cur = Resume.begin(),
		iter_end = Resume.end(); iter_cur != iter_end; ++iter_cur)
	{
		iter_cur->State = DOWNLOADING;
	}
}
