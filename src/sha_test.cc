#include <fstream>
#include <iostream>
#include <string>

#include "sha.h"

int main(int argc, char * argv[])
{
	sha SHA;

	char message[4096*128];
	std::string messageDigest;

	std::string inFile = "American Psycho.avi";
	std::string outFile = "blarg.txt";

	std::ifstream fin(inFile.c_str());
	std::ofstream fout(outFile.c_str());

	while(true){

		fin.read(message, 4096*128);

		//eof if gcount() == 0
		if(fin.gcount() == 0){
			break;
		}

		/*
		enuSHA1,
		enuSHA160 = enuSHA1,
		enuSHA224,
		enuSHA256,
		enuSHA384,
		enuSHA512,
		*/
		messageDigest = SHA.GetHash(sha::enuSHA160, (const unsigned char *)message, fin.gcount());

		fout << messageDigest << "\n";
	}

	return 0;
}
