#ifndef H_HASH
#define H_HASH

//std
#include <string>

#include "sha.h"
#include "global.h"

class hash
{
public:
	hash();
	/*
	hash_file - hash a whole file and return the message digest
	*/
	const std::string hash_file(std::string filePath);

private:
	sha SHA;
};
#endif
