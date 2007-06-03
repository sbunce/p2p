#ifndef H_HASH
#define H_HASH

//std
#include <string>

#include "sha.h"
#include "globals.h"

class hash
{
public:
	hash();
	/*
	hashFile - hash a whole file and return the message digest
	*/
	const std::string hashFile(std::string filePath);

private:
	sha SHA;
};
#endif
