#include "http.h"

http::http()
{
	if(sqlite3_open(global::DATABASE_PATH.c_str(), &sqlite3_DB) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}
	if(sqlite3_busy_timeout(sqlite3_DB, global::DB_TIMEOUT) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}
}

void http::process(const std::string & request, std::string & send)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::string get_path;

	//determine what page is being requested
	int http_pos;
	if((http_pos = request.find("HTTP")) != std::string::npos && http_pos > 5){
		get_path = request.substr(4, http_pos - 5);
		logger::debug(LOGGER_P1,"localhost requested ",get_path);
	}else{
		send += "400";
		return;
	}

	send +=
	"<html>\n"
	"<head>\n"
	"\t<title>p2p local webserver</title>\n"
	"</head>\n"
	"<body>\n";
	if(get_path == "/"){
		send +=
		"\t<b><h3>database (<a href=\"all\">all</a>)</h3></b>\n"
		"\t<ul>\n"
		"\t<li><a href=\"blacklist\">blacklist</a></li>\n"
		"\t<li><a href=\"client_preferences\">client_preferences</a></li>\n"
		"\t<li><a href=\"download\">download</a></li>\n"
		"\t<li><a href=\"hash_free\">hash_free</a></li>\n"
		"\t<li><a href=\"prime\">prime</a></li>\n"
		"\t<li><a href=\"search\">search</a></li>\n"
		"\t<li><a href=\"server_preferences\">server_preferences</a></li>\n"
		"\t<li><a href=\"share\">share</a></li>\n"
		"\t<ul>\n";
	}else if(get_path == "/all"){
		table_table("blacklist", send);
		table_table("client_preferences", send);
		table_table("download", send);
		table_table("hash_free", send);
		table_table("prime", send);
		table_table("search", send);
		table_table("server_preferences", send);
		table_table("share", send);
	}else if(get_path == "/blacklist"){
		table_table("blacklist", send);
	}else if(get_path == "/client_preferences"){
		table_table("client_preferences", send);
	}else if(get_path == "/download"){
		table_table("download", send);
	}else if(get_path == "/hash_free"){
		table_table("hash_free", send);
	}else if(get_path == "/prime"){
		table_table("prime", send);
	}else if(get_path == "/search"){
		table_table("search", send);
	}else if(get_path == "/server_preferences"){
		table_table("server_preferences", send);
	}else if(get_path == "/share"){
		table_table("share", send);
	}else{
		send += "\t<p>404</p>\n";
	}
	send += "</body>\n";
}

void http::table_table(const std::string & table_name, std::string & send)
{
	send += "\t<h3>" + table_name + "</h3>\n";
	send += "\t<table border=\"1\">\n";

	//prepare column labels
	std::string query = "SELECT * FROM ";
	query += table_name + " LIMIT 1";
	if(sqlite3_exec(sqlite3_DB, query.c_str(), table_column_name_call_back, (void *)&send, NULL) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}

	//prepare data
	query = "SELECT * FROM ";
	query += table_name;
	unsigned orig_size = send.size();
	if(sqlite3_exec(sqlite3_DB, query.c_str(), table_data_call_back, (void *)&send, NULL) != 0){
		logger::debug(LOGGER_P1,sqlite3_errmsg(sqlite3_DB));
	}
	if(orig_size == send.size()){
		//nothing added to buffer, table empty
		send += "\t<p>empty table</p>\n";
	}

	send += "\t</table><br>\n";
}
