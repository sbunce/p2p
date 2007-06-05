//boost
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/tokenizer.hpp"

//std
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

#include "serverIndex.h"

serverIndex::serverIndex()
{
	shareIndex = global::SERVER_SHARE_INDEX;
	shareName = global::SERVER_SHARE_DIRECTORY;
}

bool serverIndex::getFileInfo(int fileID, int & fileSize, std::string & filePath)
{
	std::ifstream fin(shareIndex.c_str());

	if(fin.is_open()){
		std::string temp;

		while(getline(fin, temp)){
			if(atoi(temp.substr(0, temp.find_first_of(global::DELIMITER)).c_str()) == fileID){

				boost::char_separator<char> sep(global::DELIMITER.c_str());
				boost::tokenizer<boost::char_separator<char> > tokens(temp, sep);
				boost::tokenizer<boost::char_separator<char> >::iterator iter = tokens.begin();

				//first two fields aren't needed
				iter++;
				iter++;

				fileSize = atoi((*iter++).c_str());
				filePath = *iter;

				fin.close();
				return true;
			}
		}

		fin.close();
		return false;
	}
	else{
#ifdef DEBUG
		std::cout << "error: serverIndex::getFileInfo(): can't open " << shareIndex << "\n";
#endif
		return false;
	}
}

void serverIndex::indexShare()
{
	removeMissing();
	indexShare_recurse(shareName);
}

int serverIndex::indexShare_recurse(std::string dirName)
{
	namespace fs = boost::filesystem;

	fs::path fullPath = fs::system_complete(fs::path(dirName, fs::native));

	if(!fs::exists(fullPath)){
#ifdef DEBUG
		std::cout << "error: fileIndex::indexShare(): can't locate " << fullPath.string() << "\n";
#endif
		return -1;
	}

	if(fs::is_directory(fullPath)){
		fs::directory_iterator end_iter;

		for(fs::directory_iterator dir_itr(fullPath); dir_itr != end_iter; ++dir_itr){
			try{
				if(fs::is_directory(*dir_itr)){
					//recurse to new directory
					std::string subDir;
					subDir = dirName + dir_itr->leaf() + "/";
					indexShare_recurse(subDir);
				}
				else{
					//determine size
					fs::path filePath = fs::system_complete(fs::path(dirName + dir_itr->leaf(), fs::native));
					writeEntry(fs::file_size(filePath), filePath.string());
				}
			}
			catch(std::exception & ex){
#ifdef DEBUG
				std::cout << "error: serverIndex::indexShare_recurse(): file " << dir_itr->leaf() << " caused exception " << ex.what() << "\n";
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
	std::ifstream fin(shareIndex.c_str());

	if(fin.is_open()){
		std::string temp;
		std::string fileBuffer;

		while(getline(fin, temp)){
			//attemps to open the file
			std::ifstream file(temp.substr(temp.find_last_of(global::DELIMITER) + 1).c_str());

			if(file.is_open()){
				//this file is present, save the line
				fileBuffer = fileBuffer + temp + "\n";
				continue;
			}
		}

		fin.close();

		std::ofstream fout(shareIndex.c_str());
		fout << fileBuffer;
		fout.close();

		writeIndexEntry();
	}
	else{
#ifdef DEBUG
		std::cout << "error: fileIndex::removeMissing(): couldn't open share index\n";
#endif
	}
}

void serverIndex::writeEntry(int size, std::string path)
{
	std::ifstream fin(shareIndex.c_str());

	if(fin.is_open()){

		std::string temp;
		int freeNum = 0;
		bool entryFound = false;

		while(getline(fin, temp) && !entryFound){
			//check all entries for duplicate
			if(temp.substr(temp.find_last_of(global::DELIMITER) + 1) == path){
				entryFound = true;
			}

			//check for last fileID + 1
			freeNum = atoi(temp.substr(0, temp.find_first_of(global::DELIMITER)).c_str());
			freeNum += 1;
		}
		fin.close();

		if(!entryFound){

			std::string messageDigest = Hash.hashFile(path);

			//write the share index file
			std::ostringstream entry_s;
			entry_s << freeNum << global::DELIMITER << messageDigest << global::DELIMITER << size << global::DELIMITER << path << "\n";
			std::string entry = entry_s.str();

			std::ofstream fout(shareIndex.c_str(), std::ios::app);
			if(fout.is_open()){
				fout.write(entry.c_str(), entry.length());
				fout.close();

				writeIndexEntry();
			}
			else{
#ifdef DEBUG
				std::cout << "error: serverIndex::writeEntry(): couldn't open " << shareIndex << "\n";
#endif
			}
		}
	}
	else{ //file doesn't exist, create it
		std::ofstream fout(shareIndex.c_str());

		//write the share.index entry
		writeIndexEntry();
		writeEntry(size, path);
		fout.close();
	}
}

void serverIndex::writeIndexEntry()
{
	namespace fs = boost::filesystem;

	//prepare the share index entry
	fs::path indexPath = fs::system_complete(fs::path(shareIndex, fs::native));
	int indexSize = fs::file_size(indexPath);

	std::string messageDigest = Hash.hashFile(indexPath.string());

	std::ostringstream entry_s;
	entry_s << "0" << global::DELIMITER << messageDigest << global::DELIMITER << indexSize << global::DELIMITER << indexPath.string() << "\n";

	//read share index file minus the current share index entry
	std::ifstream fin(shareIndex.c_str());
	std::string temp;
	std::string fileBuffer;
	getline(fin, temp);

	while(getline(fin, temp)){
		fileBuffer = fileBuffer + temp + "\n";
	}
	fin.close();

	//write share index plus the new share index entry
	fileBuffer = entry_s.str() + fileBuffer;
	std::ofstream fout(shareIndex.c_str());
	fout << fileBuffer;
	fout.close();
}
