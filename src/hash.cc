//std
#include <fstream>
#include <iostream>

#include "hash.h"

hash::hash()
{

}

const std::string hash::hashFile(std::string filePath)
{
	SHA.Init(global::HASH_TYPE);
	char chunk[4096*128];

	std::ifstream fin(filePath.c_str());

	if(fin.is_open()){
		while(true){

			fin.read(chunk, 4096*128);

			//eof if gcount() == 0
			if(fin.gcount() == 0){
				fin.close();
				break;
			}

			SHA.Update((const unsigned char *)chunk, fin.gcount());
		}
	}
	else{
#ifdef DEBUG
		std::cout << "error: hash::hashFile(): couldn't open " << filePath << std::endl;
#endif
	}

	SHA.End();

	return SHA.StringHash();
}

