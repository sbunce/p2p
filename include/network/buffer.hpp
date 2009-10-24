#ifndef H_NETWORK_BUFFER
#define H_NETWORK_BUFFER

//include
#include <bit_field.hpp>
#include <logger.hpp>

//standard
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iterator>
#include <string>

namespace network{
class buffer
{
public:
	//returned to indicate <something> not found
	static const int npos = -1;

	buffer():
		reserved(0),
		bytes(0),
		buf(NULL)
	{

	}

	buffer(const std::string & S):
		reserved(0),
		bytes(0),
		buf(NULL)
	{
		append(S);
	}

	buffer(const buffer & B):
		reserved(B.reserved),
		bytes(B.bytes)
	{
		buf = static_cast<unsigned char *>(std::malloc(bytes));
		std::memcpy(buf, B.buf, bytes);
	}

	~buffer()
	{
		if(buf != NULL){
			free(buf);
		}
	}

	class iterator : public std::iterator<std::random_access_iterator_tag, unsigned char>
	{
		friend class buffer;
	public:
		iterator()
		{

		}

		iterator & operator = (const iterator & rval)
		{
			pos = rval.pos;
			buf = rval.buf;
			return *this;
		}

		bool operator == (const iterator & rval) const
		{
			return pos == rval.pos;
		}

		bool operator != (const iterator & rval) const
		{
			return pos != rval.pos;
		}

		unsigned char & operator * ()
		{
			return buf[pos];
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
			return iterator(pos_tmp, buf);
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
			return iterator(pos_tmp, buf);
		}

		iterator operator + (const int rval)
		{
			return iterator(pos + rval, buf);
		}

		iterator operator + (const iterator & rval)
		{
			return iterator(pos + rval.pos, buf);
		}

		iterator operator - (const int rval)
		{
			return iterator(pos - rval, buf);
		}

		ptrdiff_t operator - (const iterator & rval)
		{
			return pos - rval.pos;
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
			return buf[pos + idx];
		}

		bool operator < (const iterator & rval) const
		{
			return pos < rval.pos;
		}

		bool operator > (const iterator & rval) const
		{
			return pos > rval.pos;
		}

		bool operator >= (const iterator & rval) const
		{
			return pos >= rval.pos;
		}

		bool operator <= (const iterator & rval) const
		{
			return pos <= rval.pos;
		}

	private:
		iterator(
			const int pos_in,
			unsigned char * buf_in
		):
			pos(pos_in),
			buf(buf_in)
		{

		}

		int pos;              //offset from start of buffer iterator is at
		unsigned char * buf; //buf that exists in buffer which spawned this iterator
	};

	//append one character to buffer
	buffer & append(const unsigned char ch)
	{
		allocate(bytes, bytes + 1);
		buf[bytes - 1] = ch;
		return *this;
	}

	//append buffer of specified size to buffer
	buffer & append(const unsigned char * buf_append, const int size)
	{
		allocate(bytes, bytes + size);
		std::memcpy(buf + bytes - size, buf_append, size);
		return *this;
	}

	//append std::string to buffer
	buffer & append(const std::string & buf_append)
	{
		append(reinterpret_cast<const unsigned char *>(buf_append.data()),
			buf_append.size());
		return *this;
	}

	//append another buffer to buffer
	buffer & append(buffer & buf_append)
	{
		append(reinterpret_cast<const unsigned char *>(buf_append.data()),
			buf_append.size());
		return *this;
	}

	//return iterator to beginning of buffer
	iterator begin() const
	{
		return iterator(0, buf);
	}

	//empties buffer
	void clear()
	{
		allocate(bytes, 0);
	}

	//return pointer to start of buffer internal storage
	unsigned char * data()
	{
		return buf;
	}

	//returns true if buffer empty
	bool empty() const
	{
		return bytes == 0;
	}

	//return iterator to end of buffer
	iterator end() const
	{
		return iterator(bytes, buf);
	}

	//erase bytes in buffer starting at index and size chars long
	void erase(const int index, const int size = npos)
	{
		if(size == npos){
			assert(index < bytes);
			allocate(bytes, index);
		}else{
			assert(index + size <= bytes);
			std::memmove(buf + index, buf + index + size, bytes - index - size);
			allocate(bytes, bytes - size);
		}
	}

	//specified amount of memory will be left allocated
	void reserve(const int size)
	{
		allocate(reserved, size);
	}

	//change the size of the buffer
	void resize(const int size)
	{
		allocate(bytes, size);
	}

	//returns size (bytes) of buffer
	int size() const
	{
		return bytes;
	}

	/*
	Returns std::string of buffer starting at index and len chars long.
	Note: This is useful for text based protocols.
	*/
	std::string str(const int index = 0, const int len = npos) const
	{
		if(len == npos){
			assert(index <= bytes);
			return std::string(reinterpret_cast<const char *>(buf + index), bytes);
		}else{
			assert(index + len <= bytes);
			return std::string(reinterpret_cast<const char *>(buf + index), len);
		}
	}

	//returns pointer to start of unused space at end of buffer
	unsigned char * tail_start() const
	{
		assert(reserved > bytes);
		return buf + bytes;
	}

	//reserve space at end of buffer
	void tail_reserve(const int size)
	{
		allocate(reserved, bytes + size);
	}

	//use space at end of buffer
	void tail_resize(const int size)
	{
		allocate(bytes, bytes + size);
	}

	//size of unused space at end of buffer
	int tail_size() const
	{
		assert(reserved >= bytes);
		return reserved - bytes;
	}

	unsigned char & operator [] (const int index)
	{
		assert(index < bytes && index >= 0);
		return buf[index];
	}

	bool operator == (const buffer & Buffer) const
	{
		if(bytes != Buffer.bytes){
			return false;
		}else{
			return std::memcmp(buf, Buffer.buf, bytes) == 0;
		}
	}

	bool operator != (const buffer & Buffer)
	{
		return !(*this == Buffer);
	}

	bool operator == (const std::string & str) const
	{
		if(bytes != str.size()){
			return false;
		}else{
			return std::memcmp(buf, str.data(), bytes) == 0;
		}
	}

	bool operator != (const std::string & str) const
	{
		return !(*this == str);
	}

private:
	int reserved;        //minimum bytes to be left allocated
	int bytes;           //how many bytes are currently allocated to buf
	unsigned char * buf; //what the buffer stores

	/*
	allocate:
		Resizes or reserves the buffer. To resize pass in bytes for the var
		parameter. To reserve pass in reserved for the var parameter. The size
		parameter is the size to resize to or reserve.
	*/
	void allocate(int & var, const int size)
	{
		if(var == size){
			//requested allocation is equal to what's already allocated
			return;
		}else{
			var = size;
			if(reserved == 0 && bytes == 0){
				//no reserve, nothing in buffer, set buf = NULL to signify empty
				if(buf != NULL){
					//free any memory currently in buffer
					free(buf);
					buf = NULL;
				}
			}else if(buf == NULL){
				//requested allocation non-zero and currently nothing is allocated
				buf = static_cast<unsigned char *>(std::malloc(var));
				assert(buf);
			}else if(var >= reserved){
				/*
				Either reserve needs to be increased, or bytes are above what is
				reserved so we need to allocate beyond what is reserved.
				*/
				buf = static_cast<unsigned char *>(std::realloc(buf, var));
				assert(buf);
			}
		}
	}
};
}//end of network namespace
#endif
