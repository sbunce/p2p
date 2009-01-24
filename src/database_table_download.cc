#include "database_table_download.h"

database::table::download::download()
: DB_Hash(DB)
{
	DB.query("CREATE TABLE IF NOT EXISTS download (hash TEXT, name TEXT, size TEXT, server TEXT)");
}

void database::table::download::complete(const std::string & hash, const int & file_size)
{
	std::ostringstream ss;
	ss << "DELETE FROM download WHERE hash = '" << hash << "' AND size = " << file_size;
	DB.query(ss.str());
}

static int lookup_hash_call_back_0(bool & entry_exists, int columns_retrieved,
	char ** response, char ** column_name)
{
	assert(response[0]);
	entry_exists = true;
	return 0;
}

bool database::table::download::lookup_hash(const std::string & hash)
{
	bool entry_exists = false;
	std::ostringstream ss;
	ss << "SELECT 1 FROM download WHERE hash = '" << hash << "'";
	DB.query(ss.str(), &lookup_hash_call_back_0, entry_exists);
	return entry_exists;
}

//std::pair<true if exists, file_path>
static int lookup_hash_call_back_1(std::pair<bool, std::string *> & info,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0]);
	info.first = true;
	*info.second = global::DOWNLOAD_DIRECTORY + response[0];
	return 0;
}

bool database::table::download::lookup_hash(const std::string & hash, std::string & path)
{
	std::pair<bool, std::string *> info(false, &path);
	std::ostringstream query;
	query << "SELECT name, size FROM download WHERE hash = '" << hash << "'";
	DB.query(query.str(), &lookup_hash_call_back_1, info);
	return info.first;
}


//boost::tuple<true if exists, path, file_size>
static int lookup_hash_call_back_2(boost::tuple<bool, std::string *, boost::uint64_t *> & info,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0] && response[1]);
	info.get<0>() = true;
	*info.get<1>() = global::DOWNLOAD_DIRECTORY + response[0];
	std::stringstream ss(response[1]);
	ss >> *info.get<2>();
	return 0;
}

bool database::table::download::lookup_hash(const std::string & hash, std::string & path, boost::uint64_t & file_size)
{
	boost::tuple<bool, std::string *, boost::uint64_t *> info(false, &path, &file_size);
	std::ostringstream query;
	query << "SELECT name, size FROM download WHERE hash = '" << hash << "'";
	DB.query(query.str(), &lookup_hash_call_back_2, info);
	return info.get<0>();
}

int database::table::download::resume_call_back(std::vector<download_info> & resume, int columns_retrieved,
	char ** response, char ** column_name)
{
	assert(response[0] && response[1] && response[2] && response[3]);
	boost::uint64_t size;
	std::stringstream ss(response[2]);
	ss >> size;
	resume.push_back(download_info(
		response[0], //hash
		response[1], //name
		size         //size
	));

	//tokenize servers
	char delims[] = ";";
	char * result = NULL;
	result = strtok(response[3], delims);
	while(result != NULL){
		resume.back().IP.push_back(result);
		result = strtok(NULL, delims);
	}
	return 0;
}

void database::table::download::resume(std::vector<download_info> & resume)
{
	DB.query("SELECT hash, name, size, server FROM download",
		this, &database::table::download::resume_call_back, resume);
}

static int is_downloading_call_back(bool & downloading, int columns_retrieved, char ** response, char ** column_name)
{
	downloading = true;
	return 0;
}

bool database::table::download::is_downloading(const std::string & hash)
{
	bool downloading = false;
	std::ostringstream query;
	query << "SELECT 1 FROM download WHERE hash = '" << hash << "'";
	DB.query(query.str(), &is_downloading_call_back, downloading);
	return downloading;
}

bool database::table::download::start(download_info & info)
{
	boost::uint64_t tree_size = hash_tree::file_size_to_tree_size(info.size);
	if(tree_size > std::numeric_limits<int>::max()){
		LOGGER << "file \"" << info.name << "\" would generate hash tree beyond max SQLite3 blob size";
		return false;
	}
	if(lookup_hash(info.hash)){
		/*
		Entry already exists in download table. This is not an error, it means the
		download is being resumed.
		*/
		return true;
	}
	DB.query("BEGIN TRANSACTION");
	DB_Hash.tree_allocate(info.hash, tree_size);
	DB_Hash.set_state(info.hash, tree_size, database::table::hash::DOWNLOADING);
	std::ostringstream query;
	query << "INSERT INTO download (hash, name, size, server) VALUES ('"
		<< info.hash << "', '" << info.name << "', '" << info.size << "', '";
	for(int x = 0; x < info.IP.size(); ++x){
		if(x+1 == info.IP.size()){
			query << info.IP[x];
		}else{
			query << info.IP[x] << ";";
		}
	}
	query << "')";
	DB.query(query.str());
	DB.query("END TRANSACTION");
	return true;
}

void database::table::download::terminate(const std::string & hash, const int & file_size)
{
	DB.query("BEGIN TRANSACTION");
	DB_Hash.delete_tree(hash, hash_tree::file_size_to_tree_size(file_size));
	std::ostringstream ss;
	ss << "DELETE FROM download WHERE hash = '" << hash << "' AND size = " << file_size;
	DB.query(ss.str());
	DB.query("END TRANSACTION");
}
