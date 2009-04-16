/*
This container is modeled after a std::string.

The purpose of this container is to be used for a network buffer. It doesn't do
copy-on-write optimization which is not thread safe. It also allows it's
underlying buffer to be exposed in a non-const way so it can be directly
written to.

This container has a size and a reserve size. The size is memory used and the
reserve size is the memory allocated for the buffer (the same as the reserve
size on a std::string).
*/
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

	//bidirectional iterator
	class iterator
	{
		friend class buffer;
	public:
		iterator(){}
		const iterator & operator = (const iterator & rval)
		{
			pos = rval.pos;
			buff = rval.buff;
			return *this;
		}
		bool operator == (const iterator & rval)
		{
			return pos == rval.pos;
		}
		bool operator != (const iterator & rval)
		{
			return pos != rval.pos;
		}
		iterator & operator ++ ()
		{
			++pos;
			return *this;
		}
		iterator operator ++ (int)
		{
			int pos_tmp = pos;
			++pos;
			return iterator(pos_tmp, buff);
		}
		iterator & operator -- ()
		{
			--pos;
			return *this;
		}
		iterator operator -- (int)
		{
			int pos_tmp = pos;
			--pos;
			return iterator(pos_tmp, buff);
		}
		unsigned char & operator * ()
		{
			return buff[pos];
		}
		iterator operator + (const int rval)
		{
			return iterator(pos + rval, buff);
		}
		iterator operator + (const iterator & rval)
		{
			return iterator(pos + rval.pos, buff);
		}
		iterator operator - (const int rval)
		{
			return iterator(pos - rval, buff);
		}
		iterator operator - (const iterator & rval)
		{
			return iterator(pos - rval.pos, buff);
		}
		iterator & operator += (const int rval)
		{
			pos += rval;
			return *this;
		}
		iterator & operator += (const iterator & rval)
		{
			pos += rval.pos;
			return *this;
		}
		iterator & operator -= (const int rval)
		{
			pos -= rval;
			return *this;
		}
		iterator & operator -= (const iterator & rval)
		{
			pos -= rval.pos;
			return *this;
		}
		unsigned char & operator [] (const int idx)
		{
			return buff[pos + idx];
		}
		bool operator < (const iterator & rval)
		{
			return pos < rval.pos;
		}

	private:
		iterator(
			const int pos_in,
			unsigned char * buff_in
		):
			pos(pos_in),
			buff(buff_in)
		{}

		int pos;              //offset from start of buffer iterator is at
		unsigned char * buff; //buff that exists in buffer which spawned this iterator
	};

	//appends 1 byte to buffer
	buffer & append(const unsigned char char_append)
	{
		allocate(bytes, bytes + 1);
		buff[bytes - 1] = char_append;
		return *this;
	}

	//appends specified number of bytes from buff_append
	buffer & append(const unsigned char * buff_append, const size_t size)
	{
		allocate(bytes, bytes + size);
		std::memcpy(buff + bytes - size, buff_append, size);
		return *this;
	}

	//returns iterator to first unsigned char of buffer
	iterator begin()
	{
		return iterator(0, buff);
	}

	//clears contents of buffer
	void clear()
	{
		allocate(bytes, 0);
	}

	//returns non NULL terminated string
	unsigned char * data()
	{
		return buff;
	}

	//returns true if no data in buffer
	bool empty()
	{
		return bytes == 0;
	}

	//returns iterator to one past last unsigned char of buffer
	iterator end()
	{
		return iterator(bytes, buff);
	}

	/*
	Erase bytes starting at index, ending at index + length. If no length
	specified bytes erased starting at index to end of buffer.
	*/
	void erase(const size_t index, const size_t length = npos)
	{
		if(length == npos){
			assert(index < bytes);
			allocate(bytes, index);
		}else{
			assert(index + length <= bytes);
			std::memmove(buff + index, buff + index + length, bytes - index - length);
			allocate(bytes, bytes - length);
		}
	}

	//find substring starting at index
	int find(const unsigned char * str, const int length, int index = 0)
	{
		assert(index + length < bytes);
		unsigned char * pos = std::search(
			(unsigned char *)(buff + index), (unsigned char *)(buff + bytes),
			(unsigned char *)str, (unsigned char *)(str + length)
		);

		if(pos == (unsigned char *)(str + length)){
			//substring not found
			return npos;
		}else{
			return (int)(pos - buff);
		}
	}

	//keeps a specified minimum number of bytes allocated
	void reserve(const size_t size)
	{
		allocate(reserved, size);
	}

	//makes buffer the specified size, memory in buffer left uninitialized
	void resize(const size_t size)
	{
		allocate(bytes, size);
	}

	size_t size()
	{
		return bytes;
	}

	//allows access to individual bytes
	unsigned char & operator [] (const size_t index)
	{
		assert(index < bytes);
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
