#ifndef H_BUFFER
#define H_BUFFER

//std
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

class buffer
{
public:
	buffer():
		reserved(0),
		bytes(0),
		buff(NULL)
	{}

	buffer(const buffer & Buffer):
		reserved(Buffer.reserved),
		bytes(Buffer.bytes)
	{
		buff = (unsigned char *)malloc(bytes);
		std::memcpy(buff, Buffer.buff, bytes);
	}

	~buffer()
	{
		if(buff != NULL){
			free(buff);
		}
	}

	//appends 1 byte to buffer
	buffer & append(const unsigned char & char_append)
	{
		allocate(bytes, bytes + 1);
		buff[bytes - 1] = char_append;
		return *this;
	}

	//appends specified number of bytes from buff_append
	buffer & append(const unsigned char * buff_append, const size_t & size)
	{
		allocate(bytes, bytes + size);
		std::memcpy(buff + bytes - size, buff_append, size);
		return *this;
	}

	//clears contents of buffer
	void clear()
	{
		allocate(bytes, 0);
	}

	//returns non NULL terminated string
	const char * data()
	{
		return (const char *)buff;
	}

	//returns true if no data in buffer
	bool empty()
	{
		return bytes == 0;
	}

	//erase bytes starting at index, ending at index + length
	void erase(const size_t & index, const size_t & length)
	{
		assert(index + length <= bytes);
		std::memmove(buff + index, buff + index + length, bytes - index - length);
		allocate(bytes, bytes - length);
	}

	//returns location of start of first str if it exists in buffer
	size_t find(const char * str)
	{
		const char * str_walk = str;
		size_t start_of_match = npos;
		for(size_t x=0; x < bytes; ++x){
			if(*str_walk == 0){
				break;
			}
			if(buff[x] == *str_walk){
				++str_walk;
				if(start_of_match == npos){
					start_of_match = x;
				}
			}else{
				str_walk = str;
			}
		}
		return start_of_match;
	}

	//keeps a specified minimum number of bytes allocated
	void reserve(const size_t & size)
	{
		allocate(reserved, size);
	}

	const size_t & size()
	{
		return bytes;
	}

	//allows access to individual bytes
	unsigned char & operator [] (const size_t & index)
	{
		assert(index < bytes || index < reserved);
		return buff[index];
	}

	//returned to indicate <something> not found
	static const size_t npos = -1;

private:
	/*
	Used for both reserve and normal allocation. When using to reserve pass in
	reserved for var parameter. When using to allocate normally pass in bytes for
	var parameter.

	The size parameter is the desired total size to reserve, or allocate
	normally, which depends on how this function is being used.
	*/
	void allocate(size_t & var, const size_t & size)
	{
		if(var == size){
			return;
		}else{
			var = size;
			if(reserved == 0 && bytes == 0){
				if(buff != NULL){
					free(buff);
					buff = NULL;
				}
			}else if(buff == NULL){
				buff = (unsigned char *)std::malloc(var);
				assert(buff);
			}else if(var > reserved){
				buff = (unsigned char *)std::realloc(buff, var);
				assert(buff);
			}
		}
	}

	size_t reserved;      //minimum bytes to be left allocated
	size_t bytes;         //how many bytes are currently allocated to buff
	unsigned char * buff; //holds the string
};
#endif
