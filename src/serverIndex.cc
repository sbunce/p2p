//boost
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

//std
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "serverIndex.h"

serverIndex::serverIndex()
{
	//create the share directory if it doesn't exist
	boost::filesystem::create_directory(global::SERVER_SHARE_DIRECTORY);

	int returnCode;
	if((returnCode = sqlite3_open(global::DATABASE_PATH.c_str(), &sqlite3_DB)) != 0){
#ifdef DEBUG
		std::cout << "error: serverIndex::serverIndex() #1 failed with sqlite3 error " << returnCode << "\n";
#endif
	}
	if((returnCode = sqlite3_busy_timeout(sqlite3_DB, 1000)) != 0){
#ifdef DEBUG
		std::cout << "error: serverIndex::serverIndex() #2 failed with sqlite3 error " << returnCode << "\n";
#endif
	}
	if((returnCode = sqlite3_exec(sqlite3_DB, "CREATE TABLE IF NOT EXISTS share (ID INTEGER PRIMARY KEY AUTOINCREMENT, hash TEXT, size INTEGER, path TEXT)", NULL, NULL, NULL)) != 0){
#ifdef DEBUG
		std::cout << "error: serverIndex::serverIndex() #3 failed with sqlite3 error " << returnCode << "\n";
#endif
	}
	if((returnCode = sqlite3_exec(sqlite3_DB, "CREATE UNIQUE INDEX IF NOT EXISTS pathIndex ON share (path)", NULL, NULL, NULL)) != 0){
#ifdef DEBUG
		std::cout << "error: serverIndex::serverIndex() #4 failed with sqlite3 error " << returnCode << "\n";
#endif
	}

	indexing = false;
}

void serverIndex::addEntry(const int & size, const std::string & path)
{
	std::ostringstream query;
	addEntry_entryExists = false;

	//determine if the entry already exists
	query << "SELECT * FROM share WHERE path LIKE \"" << path << "\" LIMIT 1";

	int returnCode;
	if((returnCode = sqlite3_exec(sqlite3_DB, query.str().c_str(), addEntry_callBack_wrapper, (void *)this, NULL)) != 0){
#ifdef DEBUG
		std::cout << "error: serverIndex::addEntry() #1 failed with sqlite3 error " << returnCode << "\n";
#endif
	}

	if(!addEntry_entryExists){
		query.str("");
		query << "INSERT INTO share (hash, size, path) VALUES ('" << Hash.hashFile(path) << "', '" << size << "', '" << path << "')";

		if((returnCode = sqlite3_exec(sqlite3_DB, query.str().c_str(), NULL, NULL, NULL)) != 0){
#ifdef DEBUG
			std::cout << "error: serverIndex::addEntry() #2 failed with sqlite3 error " << returnCode << "\n";
#endif
		}
	}
}

void serverIndex::addEntry_callBack(int & columnsRetrieved, char ** queryResponse, char ** columnName)
{
	addEntry_entryExists = true;
}

bool serverIndex::getFileInfo(const int & file_ID, int & fileSize, std::string & filePath)
{
	std::ostringstream query;
	getFileInfo_entryExists = false;

	//locate the record
	query << "SELECT size, path FROM share WHERE ID LIKE \"" << file_ID << "\" LIMIT 1";

	int returnCode;
	if((returnCode = sqlite3_exec(sqlite3_DB, query.str().c_str(), getFileInfo_callBack_wrapper, (void *)this, NULL)) != 0){
#ifdef DEBUG
		std::cout << "error: serverIndex::getFileInfo() failed with sqlite3 error " << returnCode << "\n";
#endif
	}

	if(getFileInfo_entryExists){
		fileSize = getFileInfo_fileSize;
		filePath = getFileInfo_filePath;
		return true;
	}
	else{
		return false;
	}
}

void serverIndex::getFileInfo_callBack(int & columnsRetrieved, char ** queryResponse, char ** columnName)
{
	getFileInfo_entryExists = true;
	getFileInfo_fileSize = atoi(queryResponse[0]);
	getFileInfo_filePath.assign(queryResponse[1]);
}

bool serverIndex::is_indexing()
{
	return indexing;
}

void serverIndex::indexShare_thread()
{
	while(true){
		indexing = true;
		removeMissing();
		indexShare_recurse(global::SERVER_SHARE_DIRECTORY);
		indexing = false;
		sleep(global::SHARE_REFRESH);
	}
}

int serverIndex::indexShare_recurse(const std::string directoryName)
{
	namespace fs = boost::filesystem;

	fs::path fullPath = fs::system_complete(fs::path(directoryName, fs::native));

	if(!fs::exists(fullPath)){
#ifdef DEBUG
		std::cout << "error: fileIndex::indexShare(): can't locate " << fullPath.string() << "\n";
#endif
		return -1;
	}

	if(fs::is_directory(fullPath)){

		fs::directory_iterator end_iter;

		for(fs::directory_iterator directory_iter(fullPath); directory_iter != end_iter; directory_iter++){
			try{
				if(fs::is_directory(*directory_iter)){
					//recurse to new directory
					std::string subDirectory;
					subDirectory = directoryName + directory_iter->leaf() + "/";
					indexShare_recurse(subDirectory);
				}
				else{
					//determine size
					fs::path filePath = fs::system_complete(fs::path(directoryName + directory_iter->leaf(), fs::native));
					addEntry(fs::file_size(filePath), filePath.string());
				}
			}
			catch(std::exception & ex){
#ifdef DEBUG
				std::cout << "error: serverIndex::indexShare_recurse(): file " << directory_iter->leaf() << " caused exception " << ex.what() << "\n";
#endif
			}
		}
	}
	else{
#ifdef DEBUG
		std::cout << "error: fileIndex::indexShare(): index points to file when it should point at directory\n";    
#endif
	}

	return 0;
}

void serverIndex::removeMissing()
{
	int returnCode;
	if((returnCode = sqlite3_exec(sqlite3_DB, "SELECT path FROM share", removeMissing_callBack_wrapper, (void *)this, NULL)) != 0){
#ifdef DEBUG
		std::cout << "error: serverIndex::removeMissing() failed with sqlite3 error " << returnCode << "\n";
#endif
	}
}

void serverIndex::removeMissing_callBack(int & columnsRetrieved, char ** queryResponse, char ** columnName)
{
	std::fstream temp(queryResponse[0]);

	if(temp.is_open()){
		temp.close();
	}
	else{
		std::ostringstream query;
		query << "DELETE FROM share WHERE path = \"" << queryResponse[0] << "\"";

		int returnCode;
		if((returnCode = sqlite3_exec(sqlite3_DB, query.str().c_str(), NULL, NULL, NULL)) != 0){
#ifdef DEBUG
			std::cout << "error: serverIndex::removeMissing_callBack() failed with sqlite3 error " << returnCode << "\n";
#endif
		}
	}
}

void serverIndex::start()
{
	boost::thread T(boost::bind(&serverIndex::indexShare_thread, this));
}

