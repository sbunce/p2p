#include <string>
#include <vector>

#include "digest_DB.h"

int main(int argc, char *argv[])
{
	digest_DB Digest_DB;

	std::vector<std::string> myVec;
	myVec.push_back("1111111111111111111111111111111111111111");
	myVec.push_back("2222222222222222222222222222222222222222");
	myVec.push_back("3333333333333333333333333333333333333333");
	myVec.push_back("4444444444444444444444444444444444444444");
	myVec.push_back("5555555555555555555555555555555555555555");

	Digest_DB.addDigests("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", myVec);

	return 0;
}

