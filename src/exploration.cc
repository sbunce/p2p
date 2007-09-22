//boost
#include <boost/tokenizer.hpp>

//std
#include <iostream>
#include <iomanip>
#include <sstream>

#include "exploration.h"

exploration::exploration()
{
	int returnCode;
	if((returnCode = sqlite3_open(global::DATABASE_PATH.c_str(), &sqlite3_DB)) != 0){
#ifdef DEBUG
		std::cout << "error: expoloration::exploration() #1 failed with sqlite3 error " << returnCode << "\n";
#endif
	}
	if((returnCode = sqlite3_busy_timeout(sqlite3_DB, 1000)) != 0){
#ifdef DEBUG
		std::cout << "error: expoloration::exploration() #2 failed with sqlite3 error " << returnCode << "\n";
#endif
	}
	if((returnCode = sqlite3_exec(sqlite3_DB, "CREATE TABLE IF NOT EXISTS search (hash TEXT, name TEXT, size INTEGER, server_IP TEXT, file_ID TEXT)", NULL, NULL, NULL)) != 0){
#ifdef DEBUG
		std::cout << "error: expoloration::exploration() #3 failed with sqlite3 error " << returnCode << "\n";
#endif
	}
	if((returnCode = sqlite3_exec(sqlite3_DB, "CREATE UNIQUE INDEX IF NOT EXISTS searchHashIndex ON search (hash)", NULL, NULL, NULL)) != 0){
#ifdef DEBUG
		std::cout << "error: expoloration::exploration() #4 failed with sqlite3 error " << returnCode << "\n";
#endif
	}
	if((returnCode = sqlite3_exec(sqlite3_DB, "CREATE INDEX IF NOT EXISTS searchNameIndex ON search (name)", NULL, NULL, NULL)) != 0){
#ifdef DEBUG
		std::cout << "error: expoloration::exploration() #5 failed with sqlite3 error " << returnCode << "\n";
#endif
	}
}

void exploration::asteriskToPercent(std::string & searchWord)
{
	for(std::string::iterator iter0 = searchWord.begin(); iter0 != searchWord.end(); ++iter0){
		if(*iter0 == '*'){
			*iter0 = '%';
		}
	}
}

void exploration::search(std::string & searchWord, std::list<infoBuffer> & info)
{
	searchResults.clear();
	asteriskToPercent(searchWord);

#ifdef DEBUG
	std::cout << "info: exploration::search(): search entered: \"" << searchWord << "\"\n";
#endif

	std::ostringstream query;
	if(!searchWord.empty()){
		query << "SELECT * FROM search WHERE name LIKE \"%" << searchWord << "%\"";

		int returnCode;
		if((returnCode = sqlite3_exec(sqlite3_DB, query.str().c_str(), search_callBack_wrapper, (void *)this, NULL)) != 0){
#ifdef DEBUG
			std::cout << "error: expoloration::search() failed with sqlite3 error " << returnCode << "\n";
#endif
		}

		info.clear();
		info.splice(info.end(), searchResults);
	}
}

void exploration::search_callBack(int & columnsRetrieved, char ** queryResponse, char ** columnName)
{
	infoBuffer temp;
	temp.hash.append(queryResponse[0]);
	temp.fileName.append(queryResponse[1]);
	temp.fileSize.append(queryResponse[2]);

	std::string undelim_server_IP(queryResponse[3]);
	std::string undelim_file_ID(queryResponse[4]);

	boost::char_separator<char> sep(",");
	boost::tokenizer<boost::char_separator<char> > server_IP_tokens(undelim_server_IP, sep);
	boost::tokenizer<boost::char_separator<char> >::iterator server_IP_iter = server_IP_tokens.begin();
	boost::tokenizer<boost::char_separator<char> > file_ID_tokens(undelim_file_ID, sep);
	boost::tokenizer<boost::char_separator<char> >::iterator file_ID_iter = file_ID_tokens.begin();	

	while(server_IP_iter != server_IP_tokens.end()){
		temp.server_IP.push_back(*server_IP_iter++);
		temp.file_ID.push_back(*file_ID_iter++);
	}

	searchResults.push_back(temp);
}

