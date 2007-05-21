//std
#include <fstream>
#include <iostream>

#include "hash.h"

hash::hash()
{

}

const std::string & hash::hashFile(std::string filePath)
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

/*
	//call these three in order if you want to load chunk-by-chunk...
	void Init(SHA_TYPE type);
	//these two throw a std::runtime_error if the type is not defined
	void Update(const sha_byte *data, size_t len); //call as many times as needed
	void End();

	//or call this one if you only have one chunk of data
	const std::string &GetHash(SHA_TYPE type, const sha_byte* data, size_t len);

	//call one of these routines to access the hash
	//these throw a std::runtime_error if End has not been called
	const char *HexHash();            //NULL terminated
	const std::string &StringHash();
	const char *RawHash(int &length); //NO NULL termination! size stored in 'length'
*/

