//boost
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/tokenizer.hpp"

//std
#include <fstream>
#include <iostream>
#include <sstream>

#include "clientIndex.h"

clientIndex::clientIndex()
{
	indexName = global::CLIENT_DOWNLOAD_INDEX;
	downloadDirectory = global::CLIENT_DOWNLOAD_DIRECTORY;
}

void clientIndex::downloadComplete(std::string messageDigest_in)
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(Mutex);

	std::ifstream fin(indexName.c_str());
	std::string fileBuffer;

	if(fin.is_open()){
		std::string temp;
		std::string oneLine;

		while(getline(fin, oneLine)){
			temp = oneLine;

			std::string messageDigest = temp.substr(0, temp.find_first_of(global::DELIMITER));

			//save all entries except for the one specified in parameter to function
			if(messageDigest_in != messageDigest){
				fileBuffer = fileBuffer + oneLine + "\n";
			}
			else{
				continue;
			}
		}

		fin.close();

		std::ofstream fout(indexName.c_str());
		fout << fileBuffer;
		fout.close();
	}
	else{
#ifdef DEBUG
		std::cout << "error: clientIndex::downloadComplete(): couldn't open " << indexName << std::endl;
#endif
	}

	}//end lock scope
}

std::string clientIndex::getFilePath(std::string messageDigest_in)
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(Mutex);

	std::ifstream fin(indexName.c_str());

	if(fin.is_open()){
		std::string temp;

		while(getline(fin, temp)){

			boost::char_separator<char> sep(global::DELIMITER.c_str());
			boost::tokenizer<boost::char_separator<char> > tokens(temp, sep);
			boost::tokenizer<boost::char_separator<char> >::iterator iter = tokens.begin();

			std::string messageDigest = *iter++;

			if(messageDigest_in == messageDigest){

				std::string fileName = *iter++;
				std::string filePath;
				filePath = downloadDirectory + fileName;
				return filePath;
			}
		}

		fin.close();
	}
	else{
#ifdef DEBUG
		std::cout << "error: clientIndex::getFilePath(): can't open " << indexName << std::endl;
#endif
	}

#ifdef DEBUG
		std::cout << "error: clientIndex::getFilePath(): did not return a file path" << indexName << std::endl;
#endif

	}//end lock scope
}

void clientIndex::initialFillBuffer(std::vector<clientBuffer> & sendBuffer)
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(Mutex);

	std::ifstream fin(indexName.c_str());

	if(fin.is_open()){
		std::string temp;

		while(getline(fin, temp)){

			boost::char_separator<char> sep(global::DELIMITER.c_str());
			boost::tokenizer<boost::char_separator<char> > tokens(temp, sep);
			boost::tokenizer<boost::char_separator<char> >::iterator iter = tokens.begin();

			std::string messageDigest = *iter++;
			std::string fileName = *iter++;
			int fileSize = atoi((*iter++).c_str());
			std::string server_IP = *iter++;
			int file_ID = atoi((*iter).c_str());

			//fill the bufferSubElement
			clientBuffer temp_cb;
			temp_cb.messageDigest = messageDigest;
			temp_cb.server_IP = server_IP;
			temp_cb.socketfd = -1;
			temp_cb.file_ID = file_ID;

//DEBUG, add in function to determine blockcount after resuming download
			temp_cb.blockCount = 0 * global::BUFFER_SIZE;
			temp_cb.fileSize = fileSize;
			temp_cb.fileName = fileName;
			temp_cb.filePath = downloadDirectory + fileName;
			temp_cb.lastBlock = fileSize/(global::BUFFER_SIZE - global::CONTROL_SIZE);
			temp_cb.lastBlockSize = temp_cb.fileSize % (global::BUFFER_SIZE - global::CONTROL_SIZE) + global::CONTROL_SIZE;

			sendBuffer.push_back(temp_cb);
		}

		fin.close();
	}
	else{
#ifdef DEBUG
		std::cout << "error: clientIndex::initialFillBuffer(): can't open " << indexName << std::endl;
#endif
	}

	}//end lock scope
}

int clientIndex::startDownload(std::string messageDigest, std::string server_IP, std::string file_ID, std::string fileSize, std::string fileName)
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(Mutex);

	std::ifstream fin(indexName.c_str());

	//if the download database file doesn't exist then create it
	if(!fin.is_open()){
		std::ofstream fout(indexName.c_str());
		fout.close();
		fin.close();
		fin.open(indexName.c_str());
	}

	if(fin.is_open()){
		std::string temp;
		bool entryFound = false;

		while(getline(fin, temp) && !entryFound){
			//check all entries for duplicate
			if(temp.substr(temp.find_last_of(global::DELIMITER) + 1) == fileName){
				entryFound = true;
#ifdef DEBUG
				std::cout << "error: clientIndex::startDownload(): file is already downloading" << std::endl;
#endif
				return -1;
			}
		}
		fin.close();

		if(!entryFound){
			//write the share index file
			std::ostringstream entry_s;
			entry_s << messageDigest << global::DELIMITER << fileName << global::DELIMITER << fileSize << global::DELIMITER << server_IP << global::DELIMITER << file_ID << "\n";
			std::string entry = entry_s.str();

			std::ofstream fout(indexName.c_str(), std::ios::app);
			if(fout.is_open()){
				fout.write(entry.c_str(), entry.length());
				fout.close();
			}
			else{
#ifdef DEBUG
				std::cout << "error: clientIndex::startDownload(): can't open " << indexName << std::endl;
#endif
			}
		}
	}
	else{
#ifdef DEBUG
		std::cout << "error: clientIndex::startDownload(): can't open " << indexName << std::endl;
#endif
	}

	return 0;

	}//end lock scope
}
