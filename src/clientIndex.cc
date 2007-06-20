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

void clientIndex::terminateDownload(std::string messageDigest_in)
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
		std::cout << "error: clientIndex::downloadComplete(): couldn't open " << indexName << "\n";
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
		std::cout << "error: clientIndex::getFilePath(): can't open " << indexName << "\n";
#endif
	}

#ifdef DEBUG
		std::cout << "error: clientIndex::getFilePath(): did not return a file path" << indexName << "\n";
#endif

	}//end lock scope
}

bool clientIndex::initialFillBuffer(std::vector<download> & downloadBuffer, std::vector<std::deque<download::serverElement *> > & serverHolder)
{
	namespace fs = boost::filesystem;

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
			fs::path filePath_path = fs::system_complete(fs::path(global::CLIENT_DOWNLOAD_DIRECTORY + fileName, fs::native));
			std::string filePath = filePath_path.string();
			int fileSize = atoi((*iter++).c_str());

			int currentBytes = fs::file_size(filePath_path); //get the size of the partial file
			int blockCount = currentBytes / (global::BUFFER_SIZE - global::CONTROL_SIZE);
			int lastBlock = fileSize/(global::BUFFER_SIZE - global::CONTROL_SIZE);
			int lastBlockSize = fileSize % (global::BUFFER_SIZE - global::CONTROL_SIZE) + global::CONTROL_SIZE;
			int lastSuperBlock = lastBlock / global::SUPERBLOCK_SIZE;
			int currentSuperBlock = currentBytes / ( (global::BUFFER_SIZE - global::CONTROL_SIZE) * global::SUPERBLOCK_SIZE );

			download resumedDownload(
				messageDigest,
				fileName,
				filePath,
				fileSize,
				blockCount,
				lastBlock,
				lastBlockSize,
				lastSuperBlock,
				currentSuperBlock
			);

			//add known servers associated with this download
			while(iter != tokens.end()){
				download::serverElement * SE = new download::serverElement;
				SE->server_IP = *iter++;
				SE->socketfd = -1;
				SE->file_ID = atoi((*iter++).c_str());
				resumedDownload.Server.push_back(SE);

				//add the server to the server vector if it exists, otherwise make a new one and add it to serverHolder
				bool found = false;
				for(std::vector<std::deque<download::serverElement *> >::iterator iter0 = serverHolder.begin(); iter0 != serverHolder.end(); iter0++){

					if(iter0->front()->server_IP == SE->server_IP){
						//all servers in the vector must have the same socket
						SE->socketfd = iter0->front()->socketfd;
						iter0->push_back(SE);
						found = true;
					}
				}

				if(!found){
					//make a new server deque for the server and add it to the serverHolder
					std::deque<download::serverElement *> newServer;
					newServer.push_back(SE);
					serverHolder.push_back(newServer);
				}
			}

			downloadBuffer.push_back(resumedDownload);
		}

		fin.close();

		return true;
	}
	else{
#ifdef DEBUG
		std::cout << "error: clientIndex::initialFillBuffer(): can't open " << indexName << "\n";
#endif
	}

	}//end lock scope

	return false;
}

int clientIndex::startDownload(exploration::infoBuffer info)
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
			if(temp.substr(temp.find_last_of(global::DELIMITER) + 1) == info.fileName){
				entryFound = true;
#ifdef DEBUG
				std::cout << "error: clientIndex::startDownload(): file is already downloading\n";
#endif
				return -1;
			}
		}
		fin.close();

		if(!entryFound){
			//write the share index file
			std::ostringstream entry_s;
			entry_s << info.messageDigest << global::DELIMITER << info.fileName << global::DELIMITER << info.fileSize_bytes;

			for(int x=0; x<info.server_IP.size(); x++){
				entry_s << global::DELIMITER << info.server_IP.at(x) << global::DELIMITER << info.file_ID.at(x);
			}

			entry_s << "\n";

			std::ofstream fout(indexName.c_str(), std::ios::app);
			if(fout.is_open()){
				fout.write(entry_s.str().c_str(), entry_s.str().length());
				fout.close();
			}
			else{
#ifdef DEBUG
				std::cout << "error: clientIndex::startDownload(): can't open " << indexName << "\n";
#endif
			}
		}
	}
	else{
#ifdef DEBUG
		std::cout << "error: clientIndex::startDownload(): can't open " << indexName << "\n";
#endif
	}

	return 0;

	}//end lock scope
}
