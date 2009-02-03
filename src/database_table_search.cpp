#include "database_table_search.hpp"

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

void database::table::search::run_search(std::string & search_term,
	std::vector<download_info> & search_results)
{
	run_search(search_term, search_results, DB);
}

void database::table::search::run_search(std::string & search_term,
	std::vector<download_info> & search_results, database::connection & DB)
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

void database::table::search::get_servers(const std::string & hash,
	std::vector<std::string> & servers)
{
	get_servers(hash, servers, DB);
}

void database::table::search::get_servers(const std::string & hash,
	std::vector<std::string> & servers, database::connection & DB)
{
	std::stringstream ss;
	ss << "SELECT server FROM search WHERE hash = '" << hash << "'";
	DB.query(ss.str(), &get_servers_call_back, servers);
}
