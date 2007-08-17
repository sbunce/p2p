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
	if((returnCode = sqlite3_exec(sqlite3_DB, "CREATE TABLE IF NOT EXISTS downloads (hash TEXT, name TEXT, size INTEGER, server_IP TEXT, file_ID TEXT)", NULL, NULL, NULL)) != 0){
#ifdef DEBUG
		std::cout << "error: clientIndex::clientIndex() #3 failed with sqlite3 error " << returnCode << "\n";
#endif
	}
	if((returnCode = sqlite3_exec(sqlite3_DB, "CREATE UNIQUE INDEX IF NOT EXISTS downloadsHashIndex ON downloads (hash)", NULL, NULL, NULL)) != 0){
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
	query << "SELECT name FROM downloads WHERE hash = \"" << hash << "\" LIMIT 1";

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

void clientIndex::initialFillBuffer(std::list<download> & downloadBuffer, std::list<std::list<download::serverElement *> > & serverHolder)
{
	int returnCode;
	if((returnCode = sqlite3_exec(sqlite3_DB, "SELECT * FROM downloads", initialFillBuffer_callBack_wrapper, (void *)this, NULL)) != 0){
#ifdef DEBUG
		std::cout << "error: clientIndex::initialFillBuffer() failed with sqlite3 error " << returnCode << "\n";
#endif
	}

	//do splicing of the temporary downloadBuffer and serverHolder to the actual
	serverHolder.splice(serverHolder.end(), initialFillBuffer_serverHolder);
	downloadBuffer.splice(downloadBuffer.end(), initialFillBuffer_downloadBuffer);
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

		std::string hash(queryResponse[0]);
		std::string fileName(queryResponse[1]);
		fs::path path_boost = fs::system_complete(fs::path(global::CLIENT_DOWNLOAD_DIRECTORY + fileName, fs::native));
		int size = atoi(queryResponse[2]);
		int currentBytes = fs::file_size(path_boost);
		int blockCount = currentBytes / (global::BUFFER_SIZE - global::S_CTRL_SIZE);
		int lastBlock = size/(global::BUFFER_SIZE - global::S_CTRL_SIZE);
		int lastBlockSize = size % (global::BUFFER_SIZE - global::S_CTRL_SIZE) + global::S_CTRL_SIZE;
		int lastSuperBlock = lastBlock / global::SUPERBLOCK_SIZE;
		int currentSuperBlock = currentBytes / ( (global::BUFFER_SIZE - global::S_CTRL_SIZE) * global::SUPERBLOCK_SIZE );

		download resumedDownload(
			hash,
			fileName,
			path,
			size,
			blockCount,
			lastBlock,
			lastBlockSize,
			lastSuperBlock,
			currentSuperBlock
		);

		std::string undelim_server_IP(queryResponse[3]);
		std::string undelim_file_ID(queryResponse[4]);

		boost::char_separator<char> sep(",");
		boost::tokenizer<boost::char_separator<char> > server_IP_tokens(undelim_server_IP, sep);
		boost::tokenizer<boost::char_separator<char> >::iterator server_IP_iter = server_IP_tokens.begin();
		boost::tokenizer<boost::char_separator<char> > file_ID_tokens(undelim_file_ID, sep);
		boost::tokenizer<boost::char_separator<char> >::iterator file_ID_iter = file_ID_tokens.begin();	

		//create serverElements for all servers and add them to the download
		while(server_IP_iter != server_IP_tokens.end()){
			download::serverElement * SE = new download::serverElement;
			SE->socketfd = -1;
			SE->server_IP = *server_IP_iter++;
			SE->file_ID = atoi(file_ID_iter->c_str()); file_ID_iter++;
			resumedDownload.Server.push_back(SE);

			bool ringFound = false;
			for(std::list<std::list<download::serverElement *> >::iterator iter1 = initialFillBuffer_serverHolder.begin(); iter1 != initialFillBuffer_serverHolder.end(); iter1++){

				if(!iter1->empty()){
					if(iter1->front()->server_IP == SE->server_IP){
						iter1->push_back(SE);
						ringFound = true;
						break;
					}
				}
			}

			if(!ringFound){
				std::list<download::serverElement *> newRing;
				newRing.push_back(SE);
				initialFillBuffer_serverHolder.push_back(newRing);
			}
		}

		initialFillBuffer_downloadBuffer.push_back(resumedDownload);
	}
	else{ //partial file removed, delete entry from database

		std::ostringstream query;
		query << "DELETE FROM downloads WHERE name = \"" << queryResponse[1] << "\"";

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
	query << "INSERT INTO downloads (hash, name, size, server_IP, file_ID) VALUES ('" << info.hash << "', '" << info.fileName << "', " << info.fileSize << ", '";
	for(std::vector<std::string>::iterator iter0 = info.server_IP.begin(); iter0 != info.server_IP.end(); iter0++){
		query << *iter0 << ",";
	}
	query << "', '";
	for(std::vector<std::string>::iterator iter0 = info.file_ID.begin(); iter0 != info.file_ID.end(); iter0++){
		query << *iter0 << ",";
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
	query << "DELETE FROM downloads WHERE hash = \"" << hash << "\"";

	int returnCode;
	if((returnCode = sqlite3_exec(sqlite3_DB, query.str().c_str(), NULL, NULL, NULL)) != 0){
#ifdef DEBUG
		std::cout << "error: clientIndex::terminateDownload() failed with sqlite3 error " << returnCode << "\n";
#endif
	}
}

