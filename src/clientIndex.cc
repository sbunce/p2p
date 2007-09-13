//boost
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/tokenizer.hpp>

//std
#include <fstream>
#include <iostream>
#include <list>
#include <sstream>

#include "clientIndex.h"

clientIndex::clientIndex()
{
	//create the download directory if it doesn't exist
	boost::filesystem::create_directory(global::CLIENT_DOWNLOAD_DIRECTORY);

	int returnCode;
	if((returnCode = sqlite3_open(global::DATABASE_PATH.c_str(), &sqlite3_DB)) != 0){
#ifdef DEBUG
		std::cout << "error: clientIndex::clientIndex() #1 failed with sqlite3 error " << returnCode << "\n";
#endif
	}
	if((returnCode = sqlite3_busy_timeout(sqlite3_DB, 1000)) != 0){
#ifdef DEBUG
		std::cout << "error: clientIndex::clientIndex() #2 failed with sqlite3 error " << returnCode << "\n";
#endif
	}
	if((returnCode = sqlite3_exec(sqlite3_DB, "CREATE TABLE IF NOT EXISTS download (hash TEXT, name TEXT, size INTEGER, server_IP TEXT, file_ID TEXT)", NULL, NULL, NULL)) != 0){
#ifdef DEBUG
		std::cout << "error: clientIndex::clientIndex() #3 failed with sqlite3 error " << returnCode << "\n";
#endif
	}
	if((returnCode = sqlite3_exec(sqlite3_DB, "CREATE UNIQUE INDEX IF NOT EXISTS hashIndex ON download (hash)", NULL, NULL, NULL)) != 0){
#ifdef DEBUG
		std::cout << "error: clientIndex::clientIndex() #4 failed with sqlite3 error " << returnCode << "\n";
#endif
	}
}

bool clientIndex::getFilePath(const std::string & hash, std::string & path)
{
	std::ostringstream query;
	getFilePath_entryExists = false;

	//locate the record
	query << "SELECT name FROM download WHERE hash = \"" << hash << "\" LIMIT 1";

	int returnCode;
	if((returnCode = sqlite3_exec(sqlite3_DB, query.str().c_str(), getFilePath_callBack_wrapper, (void *)this, NULL)) != 0){
#ifdef DEBUG
		std::cout << "error: clientIndex::getFilePath() failed with sqlite3 error " << returnCode << "\n";
#endif
	}

	if(getFilePath_entryExists){
		path = global::CLIENT_DOWNLOAD_DIRECTORY + getFilePath_fileName;
		return true;
	}
	else{
#ifdef DEBUG
		std::cout << "error: clientIndex::getFilePath() didn't find record\n";
#endif
		return false;
	}
}

void clientIndex::getFilePath_callBack(int & columnsRetrieved, char ** queryResponse, char ** columnName)
{
	getFilePath_entryExists = true;
	getFilePath_fileName.assign(queryResponse[0]);
}

void clientIndex::initialFillBuffer(std::list<exploration::infoBuffer> & resumedDownload)
{
	int returnCode;
	if((returnCode = sqlite3_exec(sqlite3_DB, "SELECT * FROM download", initialFillBuffer_callBack_wrapper, (void *)this, NULL)) != 0){
#ifdef DEBUG
		std::cout << "error: clientIndex::initialFillBuffer() failed with sqlite3 error " << returnCode << "\n";
#endif
	}

	resumedDownload.splice(resumedDownload.end(), initialFillBuffer_resumedDownload);
}

void clientIndex::initialFillBuffer_callBack(int & columnsRetrieved, char ** queryResponse, char ** columnName)
{
	namespace fs = boost::filesystem;

	std::string path(queryResponse[1]);
	path = global::CLIENT_DOWNLOAD_DIRECTORY + path;

	//make sure the file is present
	std::fstream fstream_temp(path.c_str());
	if(fstream_temp.is_open()){ //partial file exists, add to downloadBuffer
		fstream_temp.close();

		exploration::infoBuffer IB;
		IB.resumed = true;

		IB.hash.assign(queryResponse[0]);
		IB.fileName.assign(queryResponse[1]);
		fs::path path_boost = fs::system_complete(fs::path(global::CLIENT_DOWNLOAD_DIRECTORY + IB.fileName, fs::native));
		IB.fileSize.assign(queryResponse[2]);
		unsigned int currentBytes = fs::file_size(path_boost);
		IB.latestRequest = currentBytes / (global::BUFFER_SIZE - global::S_CTRL_SIZE);
		IB.currentSuperBlock = currentBytes / ( (global::BUFFER_SIZE - global::S_CTRL_SIZE) * global::SUPERBLOCK_SIZE );

		std::string undelim_server_IP(queryResponse[3]);
		std::string undelim_file_ID(queryResponse[4]);

		boost::char_separator<char> sep(","); //delimiter for servers and file_IDs
		boost::tokenizer<boost::char_separator<char> > server_IP_tokens(undelim_server_IP, sep);
		boost::tokenizer<boost::char_separator<char> >::iterator server_IP_iter = server_IP_tokens.begin();
		while(server_IP_iter != server_IP_tokens.end()){
			IB.server_IP.push_back(*server_IP_iter++);
		}

		boost::tokenizer<boost::char_separator<char> > file_ID_tokens(undelim_file_ID, sep);
		boost::tokenizer<boost::char_separator<char> >::iterator file_ID_iter = file_ID_tokens.begin();
		while(file_ID_iter != file_ID_tokens.end()){
			IB.file_ID.push_back(*file_ID_iter++);
		}

		initialFillBuffer_resumedDownload.push_back(IB);
	}
	else{ //partial file removed, delete entry from database

		std::ostringstream query;
		query << "DELETE FROM download WHERE name = \"" << queryResponse[1] << "\"";

		int returnCode;
		if((returnCode = sqlite3_exec(sqlite3_DB, query.str().c_str(), NULL, NULL, NULL)) != 0){
#ifdef DEBUG
		std::cout << "error: clientIndex::initialFillBuffer() failed with sqlite3 error " << returnCode << "\n";
#endif
		}
	}
}

bool clientIndex::startDownload(exploration::infoBuffer & info)
{
	std::ostringstream query;
	query << "INSERT INTO download (hash, name, size, server_IP, file_ID) VALUES ('" << info.hash << "', '" << info.fileName << "', " << info.fileSize << ", '";
	for(std::vector<std::string>::iterator iter0 = info.server_IP.begin(); iter0 != info.server_IP.end(); iter0++){

		if(iter0 + 1 != info.server_IP.end()){
			query << *iter0 << ",";
		}
		else{
			query << *iter0;
		}
	}
	query << "', '";
	for(std::vector<std::string>::iterator iter0 = info.file_ID.begin(); iter0 != info.file_ID.end(); iter0++){

		if(iter0 + 1 != info.file_ID.end()){
			query << *iter0 << ",";
		}
		else{
			query << *iter0;
		}
	}
	query << "')";

	int returnCode;
	if((returnCode = sqlite3_exec(sqlite3_DB, query.str().c_str(), NULL, NULL, NULL)) == 0){
		return true;
	}
	else{
#ifdef DEBUG
		std::cout << "error: clientIndex::startDownload() failed with sqlite3 error " << returnCode << "\n";
#endif
		return false;
	}
}

void clientIndex::terminateDownload(const std::string & hash)
{
	std::ostringstream query;
	query << "DELETE FROM download WHERE hash = \"" << hash << "\"";

	int returnCode;
	if((returnCode = sqlite3_exec(sqlite3_DB, query.str().c_str(), NULL, NULL, NULL)) != 0){
#ifdef DEBUG
		std::cout << "error: clientIndex::terminateDownload() failed with sqlite3 error " << returnCode << "\n";
#endif
	}
}

