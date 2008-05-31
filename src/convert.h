#ifndef H_CONVERT
#define H_CONVERT

//custom
#include "global.h"

//std
#include <string>

template<class T>
class convert
{
public:
	convert();

	/*
	encode - number -> bytes (encoded in big-endian)
	decode - bytes -> number
	*/
	std::string encode(const T & number);
	T decode(std::string encoded);

private:
	enum endianness { BIG_ENDIAN_ENUM, LITTLE_ENDIAN_ENUM };
	endianness ENDIANNESS;

	union num_char{
		T num;
		unsigned char byte[sizeof(T)];
	};
};

template<class T>
convert<T>::convert()
{
	//check endianness of hardware
	num_char IC = { 1 };
	if(IC.byte[0] == 1){
		ENDIANNESS = LITTLE_ENDIAN_ENUM;
	}else{
		ENDIANNESS = BIG_ENDIAN_ENUM;
	}
}

template<class T>
std::string convert<T>::encode(const T & number)
{
	num_char IC = { number };
	std::string encoded;
	if(ENDIANNESS == LITTLE_ENDIAN_ENUM){
		for(int x=sizeof(T)-1; x>=0; --x){
			encoded += IC.byte[x];
		}
	}else{
		for(int x=0; x<sizeof(T); ++x){
			encoded += IC.byte[x];
		}
	}
	return encoded;
}

template<class T>
T convert<T>::decode(std::string encoded)
{
	assert(encoded.size() == sizeof(T));
	num_char IC;
	if(ENDIANNESS == LITTLE_ENDIAN_ENUM){
		for(int x=0; x<sizeof(T); ++x){
			IC.byte[x] = (unsigned char)encoded[sizeof(T)-1-x];
		}
	}else{
		for(int x=0; x<sizeof(T); ++x){
			IC.byte[x] = (unsigned char)encoded[x];
		}
	}
	return IC.num;
}
#endif
