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
		std::string fileSize_bytes;
		std::string fileSize;

		//information unique to the server, these vectors are parallel
		std::vector<std::string> server_IP;
		std::vector<std::string> file_ID;
	};

	exploration();
	/*
	getSearchResults - retrieves the search results
	*/
	void getSearchResults(std::vector<infoBuffer> & info);
	/*
	search - searches search DB and populates searchResults
	search.db file format:
	<SHA_hash>|<fileName>|<fileSize>|<serverInfo>
	<serverInfo> = <server_IP>|<file_ID(remote)>|<serverInfo><nothing>
	<nothing> = no more to the record
	*/
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
