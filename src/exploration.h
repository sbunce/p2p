#ifndef H_EXPLORATION
#define H_EXPLORATION

//std
#include <string>
#include <vector>

#include "globals.h"

class exploration
{
public:
	//used to pass information to user interface
	class infoBuffer
	{
	public:
		std::string messageDigest;
		std::string fileName;
		std::string IP;
		std::string file_ID;
		std::string fileSize_bytes;
		std::string fileSize;
	};

	exploration();
	/*
	getSearchResults - retrieves the search results
	search           - searches search DB and populates searchResults
	*/
	void getSearchResults(std::vector<infoBuffer> & info);
	void search(std::string searchWord);

private:
	std::string searchDatabase;

	//contains the results from the last search
	std::vector<infoBuffer> searchResults;

	/*
	stringToUpper - converts a string to upper case
	*/
	void stringToUpper(std::string & str);
};
#endif
