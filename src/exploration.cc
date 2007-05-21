//boost
#include "boost/tokenizer.hpp"

//std
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>

//c standard libraries
#include <ctype.h>

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
	std::cout << "info: exploration::search(): search entered: " << searchWord << std::endl;
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

				std::string server_IP = *iter++;
				std::string file_ID = *iter++;
				std::string fileSize_bytes = *iter;
				float fileSize = atoi(fileSize_bytes.c_str());
				
				//convert fileSize from Bytes to megaBytes
				fileSize = fileSize / 1024 / 1024;

				std::ostringstream fileSize_s;

				if(fileSize < 1){
					fileSize_s << std::setprecision(2) << fileSize << " mB";
				}
				else{
					fileSize_s << (int)fileSize << " mB";
				}

				infoBuffer info;
				info.messageDigest = messageDigest;
				info.fileName = fileName;
				info.fileSize = fileSize_s.str();
				info.fileSize_bytes = fileSize_bytes;
				info.IP = server_IP;
				info.file_ID = file_ID;

				searchResults.push_back(info);
			}
		}
	}
	else{
#ifdef DEBUG
		std::cout << "error: exploration::search(): cannot open " << searchDatabase << std::endl;
#endif
	}

	fin.close();
}
