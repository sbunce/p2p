#include "http.h"

http::http()
: DB(global::DATABASE_PATH)
{

}

static int table_name_call_back(std::vector<std::string> & table_name,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0]);
	table_name.push_back(response[0]);
	return 0;
}

void http::process(const std::string & request, std::string & send)
{
	std::string get_path;

	//determine what page is being requested
	int http_pos;
	if((http_pos = request.find("HTTP")) != std::string::npos && http_pos > 5){
		get_path = request.substr(4, http_pos - 5);
		LOGGER << "localhost requested " << get_path;
	}else{
		send += "400";
		return;
	}

	//get table names
	std::string table_name_query("SELECT name FROM sqlite_master WHERE type='table' ORDER BY name");
	std::vector<std::string> table_name;
	DB.query(table_name_query, &table_name_call_back, table_name);

	send +=
	"<html>\n"
	"<head>\n"
	"\t<title>p2p local webserver</title>\n"
	"</head>\n"
	"<body>\n";
	if(get_path == "/"){
		//list all tables
		send +=
		"\t<b><h3>database tables (<a href=\"all\">all</a>)</h3></b>\n"
		"\t<ul>\n"
		"\t<li><b><a href=\"sqlite_master\">sqlite_master</a></b></li>\n";
		for(int x=0; x<table_name.size(); ++x){
			send += "\t<li><a href=\"" + table_name[x] + "\">" + table_name[x] + "</a></li>\n";
		}
		send += "\t<ul>\n";
	}else if(get_path == "/sqlite_master"){
		table_table("sqlite_master", send);
	}else if(get_path == "/all"){
		//list all tables
		table_table("sqlite_master", send);
		for(int x=0; x<table_name.size(); ++x){
			table_table(table_name[x], send);
		}
	}else{
		//attempt to find table
		get_path.erase(get_path.begin()); //erase '/'
		std::vector<std::string>::iterator iter;
		iter = std::find(table_name.begin(), table_name.end(), get_path);
		if(iter == table_name.end()){
			//no table found
			send += "\t<p>404</p>\n";
		}else{
			table_table(*iter, send);
		}
	}
	send += "</body>\n";
}

static int column_name_call_back(std::string & send, int columns_retrieved,
	char ** query_response, char ** column_name)
{
	"\t<b><tr>\n";
	for(int x=0; x<columns_retrieved; ++x){
		send += "\t<td>";
		if(column_name[x]){
			send += column_name[x];
		}
		send += "</td>\n";
	}
	send += "\t</tr></b>\n";
	return 0;
}

static int data_call_back(std::string & send, int columns_retrieved,
	char ** query_response, char ** column_name)
{
	"\t<tr>\n";
	for(int x=0; x<columns_retrieved; ++x){
		send += "\t<td>";
		if(query_response[x]){
			send += query_response[x];
		}
		send += "</td>\n";
	}
	send += "\t</tr>\n";
	return 0;
}

void http::table_table(const std::string & table_name, std::string & send)
{
	send += "\t<h3>" + table_name + "</h3>\n";
	send += "\t<table border=\"1\">\n";

	//prepare column labels
	std::string query = "SELECT * FROM " + table_name + " LIMIT 1";
	DB.query(query, &column_name_call_back, send);

	//prepare data
	query = "SELECT * FROM " + table_name;
	unsigned orig_size = send.size();
	DB.query(query, &data_call_back, send);
	if(orig_size == send.size()){
		//nothing added to buffer, table empty
		send += "\t<p>empty table</p>\n";
	}
	send += "\t</table><br>\n";
}
