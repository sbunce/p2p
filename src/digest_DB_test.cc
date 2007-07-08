#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>

#include "digest_DB.h"

int main(int argc, char *argv[])
{
	digest_DB Digest_DB;

	std::deque<std::string> myDeq1;
	myDeq1.push_back("1111111111111111111111111111111111111111");
	myDeq1.push_back("2222222222222222222222222222222222222222");
	myDeq1.push_back("3333333333333333333333333333333333333333");
	myDeq1.push_back("4444444444444444444444444444444444444444");
	myDeq1.push_back("5555555555555555555555555555555555555555");

	Digest_DB.addDigests("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", myDeq1);


	std::deque<std::string> myDeq2;
	myDeq2.push_back("6666666666666666666666666666666666666666");
	myDeq2.push_back("7777777777777777777777777777777777777777");
	myDeq2.push_back("8888888888888888888888888888888888888888");
	myDeq2.push_back("9999999999999999999999999999999999999999");
	myDeq2.push_back("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");

	Digest_DB.addDigests("YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY", myDeq2);


	std::deque<std::string> myDeq3;
	myDeq3.push_back("BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB");
	myDeq3.push_back("CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");
	myDeq3.push_back("DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD");
	myDeq3.push_back("EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE");
	myDeq3.push_back("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");

	Digest_DB.addDigests("ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ", myDeq3);


	std::deque<std::string> retrieval1;
	Digest_DB.retrieveDigests("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", retrieval1);

	for(std::deque<std::string>::iterator iter0 = retrieval1.begin(); iter0 !=retrieval1.end(); iter0++){

		std::cout << *iter0 << "\n";
	}

	Digest_DB.deleteDigests("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");

	return 0;
}

