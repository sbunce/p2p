#ifndef H_SERVERINDEX
#define H_SERVERINDEX

#include <string>

#include "globals.h"
#include "hash.h"

class serverIndex
{
public:
	/*
	indexShare   - updates the share.index file to reflect changes in ./share
	               returns 0 if success or -1 if error
	getFileInfo  - client inputs the fileID, function returns the file size and file path
	             - returns false if file doesn't exist
	*/
	serverIndex();
	/*
	share.index file format
	fileID 0 is always the share index
	<fileID>:<size>:<path>
	fileID     - arbitrary number, needed for client to request what file they want
	fileSize   - size in blocks, blocksize determined in server.cc
	filePath   - path to the file, something like ./share/file.avi
	*/
	void indexShare();
	bool getFileInfo(int fileID, int & fileSize, std::string & filePath);

private:
	std::string shareName;
	std::string shareIndex;

	hash Hash; //hash generator for blocks and whole files

	/*
	indexShare_recurse - recursively goes through directories indexing files
	removeMissing      - removes entries of missing files from share index
	writeEntry         - checks to see if entry in share.index exists, if not it writes one
	writeIndexEntry    - writes the entry for the share index(always fileID 0)
	                   - no is_open() check on this, only use when you know file opened
	*/
	int indexShare_recurse(std::string dirName);
	void removeMissing();
	void writeEntry(int size, std::string path);
	void writeIndexEntry();
};
#endif
