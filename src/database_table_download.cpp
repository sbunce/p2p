#include "database_table_download.hpp"

void database::table::download::complete(const std::string & hash, const int & file_size)
{
	complete(hash, file_size, DB);
}

void database::table::download::complete(const std::string & hash, const int & file_size, database::connection & DB)
{
	std::stringstream ss;
	ss << "DELETE FROM download WHERE hash = '" << hash << "' AND size = " << file_size;
	DB.query(ss.str());
}

static int lookup_hash_0_call_back(bool & entry_exists, int columns_retrieved,
	char ** response, char ** column_name)
{
	assert(response[0]);
	entry_exists = true;
	return 0;
}

bool database::table::download::lookup_hash(const std::string & hash)
{
	return lookup_hash(hash, DB);
}

bool database::table::download::lookup_hash(const std::string & hash, database::connection & DB)
{
	bool entry_exists = false;
	std::stringstream ss;
	ss << "SELECT 1 FROM download WHERE hash = '" << hash << "'";
	DB.query(ss.str(), &lookup_hash_0_call_back, entry_exists);
	return entry_exists;
}

//std::pair<true if exists, file_path>
static int lookup_hash_1_call_back(std::pair<bool, std::string *> & info,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0]);
	info.first = true;
	*info.second = global::DOWNLOAD_DIRECTORY + response[0];
	return 0;
}

bool database::table::download::lookup_hash(const std::string & hash, std::string & path)
{
	return lookup_hash(hash, path, DB);
}

bool database::table::download::lookup_hash(const std::string & hash, std::string & path, database::connection & DB)
{
	std::pair<bool, std::string *> info(false, &path);
	std::stringstream ss;
	ss << "SELECT name, size FROM download WHERE hash = '" << hash << "'";
	DB.query(ss.str(), &lookup_hash_1_call_back, info);
	return info.first;
}

//std::pair<true if found, file_size>
static int lookup_hash_2_call_back(std::pair<bool, boost::uint64_t *> & info,
	int columns_retrieved, char ** response, char ** column_name)
{
	info.first = true;
	std::stringstream ss(response[0]);
	ss >> *info.second;
	return 0;
}

bool database::table::download::lookup_hash(const std::string & hash, boost::uint64_t & size)
{
	return lookup_hash(hash, size, DB);
}

bool database::table::download::lookup_hash(const std::string & hash, boost::uint64_t & size, database::connection & DB)
{
	std::pair<bool, boost::uint64_t *> info(false, &size);
	std::stringstream ss;
	ss << "SELECT size FROM download WHERE hash = '" << hash << "' LIMIT 1";
	DB.query(ss.str(), &lookup_hash_2_call_back, info);
	return info.first;
}

//boost::tuple<true if exists, path, file_size>
static int lookup_hash_2_call_back(boost::tuple<bool, std::string *, boost::uint64_t *> & info,
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
	return lookup_hash(hash, path, file_size, DB);
}

bool database::table::download::lookup_hash(const std::string & hash, std::string & path, boost::uint64_t & file_size, database::connection & DB)
{
	boost::tuple<bool, std::string *, boost::uint64_t *> info(false, &path, &file_size);
	std::stringstream ss;
	ss << "SELECT name, size FROM download WHERE hash = '" << hash << "'";
	DB.query(ss.str(), &lookup_hash_2_call_back, info);
	return info.get<0>();
}

static void tokenize_servers(const std::string & servers, std::vector<std::string> & tokens)
{
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> char_sep(";");
	tokenizer T(servers, char_sep);
	tokenizer::iterator iter_cur, iter_end;
	iter_cur = T.begin();
	iter_end = T.end();
	while(iter_cur != iter_end){
		tokens.push_back(*iter_cur);
		++iter_cur;
	}
}

static int resume_call_back(std::vector<download_info> & resume_DL, int columns_retrieved,
	char ** response, char ** column_name)
{
	assert(response[0] && response[1] && response[2] && response[3]);
	boost::uint64_t size;
	std::stringstream ss(response[2]);
	ss >> size;
	resume_DL.push_back(download_info(
		response[0], //hash
		response[1], //name
		size         //size
	));

	tokenize_servers(response[3], resume_DL.back().IP);
	return 0;
}

void database::table::download::resume(std::vector<download_info> & resume_DL)
{
	resume(resume_DL, DB);
}

void database::table::download::resume(std::vector<download_info> & resume_DL, database::connection & DB)
{
	DB.query("SELECT hash, name, size, server FROM download", &resume_call_back, resume_DL);
}

static int is_downloading_call_back(bool & downloading, int columns_retrieved, char ** response, char ** column_name)
{
	downloading = true;
	return 0;
}

bool database::table::download::is_downloading(const std::string & hash, database::connection & DB)
{
	bool downloading = false;
	std::stringstream ss;
	ss << "SELECT 1 FROM download WHERE hash = '" << hash << "'";
	DB.query(ss.str(), &is_downloading_call_back, downloading);
	return downloading;
}

bool database::table::download::start(download_info & info)
{
	return start(info, DB);
}

bool database::table::download::start(download_info & info, database::connection & DB)
{
	boost::uint64_t tree_size = hash_tree::file_size_to_tree_size(info.size);
	if(tree_size > std::numeric_limits<int>::max()){
		LOGGER << "file \"" << info.name << "\" would generate hash tree beyond max SQLite3 blob size";
		return false;
	}
	if(lookup_hash(info.hash, DB)){
		/*
		Entry already exists in download table. This is not an error, it means the
		download is being resumed.
		*/
		return true;
	}
	DB.query("BEGIN TRANSACTION");
	database::table::hash::tree_allocate(info.hash, tree_size, DB);
	database::table::hash::set_state(info.hash, tree_size, database::table::hash::DOWNLOADING, DB);
	std::stringstream ss;
	ss << "INSERT INTO download (hash, name, size, server) VALUES ('"
		<< info.hash << "', '" << info.name << "', '" << info.size << "', '";
	for(int x = 0; x < info.IP.size(); ++x){
		if(x+1 == info.IP.size()){
			ss << info.IP[x];
		}else{
			ss << info.IP[x] << ";";
		}
	}
	ss << "')";
	DB.query(ss.str());
	DB.query("END TRANSACTION");
	return true;
}

void database::table::download::terminate(const std::string & hash, const int & file_size)
{
	terminate(hash, file_size, DB);
}

void database::table::download::terminate(const std::string & hash, const int & file_size, database::connection & DB)
{
	DB.query("BEGIN TRANSACTION");
	database::table::hash::delete_tree(hash, hash_tree::file_size_to_tree_size(file_size), DB);
	std::stringstream ss;
	ss << "DELETE FROM download WHERE hash = '" << hash << "' AND size = " << file_size;
	DB.query(ss.str());
	DB.query("END TRANSACTION");
}
