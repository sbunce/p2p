//boost
#include "boost/tokenizer.hpp"

//std
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>

#include "exploration.h"

exploration::exploration()
{
	searchDatabase = global::EXPLORATION_SEARCH_DATABASE;
}

void exploration::stringToUpper(std::string & str)
{
	for(int x=0; x<str.length(); x++){
		str.at(x) = toupper(str.at(x));
	}
}

void exploration::getSearchResults(std::vector<infoBuffer> & info)
{
	info.clear();

	for(int x=0; x<searchResults.size(); x++){
		info.push_back(searchResults.at(x));
	}
}

void exploration::search(std::string searchWord)
{
	searchResults.clear();
	stringToUpper(searchWord);

#ifdef DEBUG
	std::cout << "info: exploration::search(): search entered: " << searchWord << "\n";
#endif

	std::ifstream fin(searchDatabase.c_str());

	if(fin.is_open()){
		std::string temp;

		while(getline(fin, temp)){
			//retrieve components of line necessary for searching
			boost::char_separator<char> sep(global::DELIMITER.c_str());
			boost::tokenizer<boost::char_separator<char> > tokens(temp, sep);
			boost::tokenizer<boost::char_separator<char> >::iterator iter = tokens.begin();

			std::string messageDigest = *iter++;
			std::string fileName = *iter++;

			//make a temp fileName so as to not touch the case of the original
			std::string tempFileName = fileName;
			stringToUpper(tempFileName);

			//search for the word
			int location = tempFileName.rfind(searchWord.c_str(), searchWord.length());
			if(location == -1){
				continue;
			}
			else{ //found result

				infoBuffer info;

				std::string fileSize_bytes = *iter++;

				while(iter != tokens.end()){
					info.server_IP.push_back(*iter++);
					info.file_ID.push_back(*iter++);
				}

				//convert fileSize from Bytes to megaBytes
				float fileSize = atoi(fileSize_bytes.c_str());
				fileSize = fileSize / 1024 / 1024;
				std::ostringstream fileSize_s;
				if(fileSize < 1){
					fileSize_s << std::setprecision(2) << fileSize << " mB";
				}
				else{
					fileSize_s << (int)fileSize << " mB";
				}

				info.messageDigest = messageDigest;
				info.fileName = fileName;
				info.fileSize_bytes = fileSize_bytes;
				info.fileSize = fileSize_s.str();

				searchResults.push_back(info);
			}
		}
	}
	else{
#ifdef DEBUG
		std::cout << "error: exploration::search(): cannot open " << searchDatabase << "\n";
#endif
	}

	fin.close();
}
