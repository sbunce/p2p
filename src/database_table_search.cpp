#include "database_table_search.hpp"

static int check_exists_call_back(bool & exists, int columns_retrieved,
	char ** response, char ** column_name)
{
	exists = true;
	return 0;
}

database::table::search::search()
{
	/*
	There is no "CREATE VIRTUAL TABLE IF NOT EXISTS" in sqlite3 (although it may
	be added eventually). This is needed to avoid trying to create the table
	twice and causing errors.
	*/
	bool exists = false;
	DB.query("SELECT 1 FROM sqlite_master WHERE name = 'search'", &check_exists_call_back, exists);
	if(!exists){
		DB.query("CREATE VIRTUAL TABLE search USING FTS3 (hash TEXT, name TEXT, size TEXT, server TEXT)");
	}
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

static int search_call_back(std::vector<download_info> & search_results, int columns_retrieved,
	char ** response, char ** column_name)
{
	assert(response[0] && response[1] && response[2] && response[3]);
	std::stringstream ss(response[2]);
	boost::uint64_t size;
	ss >> size;
	download_info Download_Info(
		response[0], //hash
		response[1], //name
		size         //size
	);

	tokenize_servers(response[3], Download_Info.IP);
	search_results.push_back(Download_Info);
	return 0;
}

void database::table::search::run_search(std::string & search_term, std::vector<download_info> & search_results)
{
	search_results.clear();
	LOGGER << "search: " << search_term;
	if(!search_term.empty()){
		std::stringstream ss;
		char * sqlite3_sanitized = sqlite3_mprintf("%Q", search_term.c_str());
		ss << "SELECT hash, name, size, server FROM search WHERE name MATCH "
			<< sqlite3_sanitized;
		sqlite3_free(sqlite3_sanitized);
		DB.query(ss.str(), &search_call_back, search_results);
	}
}

static int get_servers_call_back(std::vector<std::string> & servers, int columns_retrieved,
	char ** response, char ** column_name)
{
	assert(response[0]);
	tokenize_servers(response[0], servers);
	return 0;
}

void database::table::search::get_servers(const std::string & hash, std::vector<std::string> & servers)
{
	std::stringstream ss;
	ss << "SELECT server FROM search WHERE hash = '" << hash << "'";
	DB.query(ss.str(), &get_servers_call_back, servers);
}
