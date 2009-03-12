/*
Buffer is very similar to a std::string. However it is guaranteed to be thread
safe for copy-by-value because there is no copy-on-write optimization.

There are also handy functions for encoding various things.
*/
#ifndef H_BUFFER
#define H_BUFFER

//std
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>

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
			}else if(var > reserved){
				buff = (unsigned char *)std::realloc(buff, var);
			}
		}
	}

	size_t reserved;      //minimum bytes to be left allocated
	size_t bytes;         //how many bytes are currently allocated to buff
	unsigned char * buff; //holds the string
};
#endif
