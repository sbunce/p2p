#include <fstream>
#include <iostream>
#include <string>

#include "sha.h"
#include "globals.h"

int main(int argc, char * argv[])
{
	sha SHA;
	SHA.Init(global::HASH_TYPE);

	std::ifstream fin("RMS at UCSD.ogg");

	if(fin.is_open()){

		std::string fileBlock;

		while(true){

			fileBlock.clear();

			//get one superBlock worth of data
			char ch;
			int bytes = 0;
			while(fin.get(ch)){
				fileBlock += ch;
				bytes++;
				if(bytes == (global::BUFFER_SIZE - global::RESPONSE_CONTROL_SIZE) * global::SUPERBLOCK_SIZE){
					break;
				}
			}

			SHA.Update((const sha_byte *) fileBlock.c_str(), fileBlock.size());
			SHA.End();

			std::cout << SHA.StringHash() << "\n";

			if(fin.gcount() == 0){
				break;
			}
		}
	}

	return 0;
}
