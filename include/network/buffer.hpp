/*
This container is modeled after a std::string.

This container doesn't do copy-on-write optimization which can make writing
thread safe code difficult. This container also exposes it's underlying buffer
in a non-const way which makes it possible to use it with send()/recv() without
extra copying.

This container offers many convenience functions for encoding data to be sent
over the network.
*/
#ifndef H_BUFFER
#define H_BUFFER

//include
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
	buffer():
		reserved(0),
		bytes(0),
		buff(NULL)
	{}

	buffer(const buffer & Buffer):
		reserved(Buffer.reserved),
		bytes(Buffer.bytes)
	{
		buff = static_cast<unsigned char *>(std::malloc(bytes));
		std::memcpy(buff, Buffer.buff, bytes);
	}

	~buffer()
	{
		if(buff != NULL){
			free(buff);
		}
	}

	//bidirectional iterator
	class iterator : public std::iterator<std::random_access_iterator_tag, unsigned char>
	{
		friend class buffer;
	public:
		iterator(){}
		iterator & operator = (const iterator & rval)
		{
			pos = rval.pos;
			buff = rval.buff;
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
			return buff[pos];
		}
		//operator -> doesn't have to be defined because ->m is not well-defined
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
			return buff[pos + idx];
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

	//append NULL terminated string, useful for plain text protocols
	buffer & append(const char * buff_append)
	{
		size_t size = std::strlen(buff_append);
		allocate(bytes, bytes + size);
		std::memcpy(buff + bytes - size, buff_append, size);
		return *this;
	}

	//append std::string
	buffer & append(const std::string & buff_append)
	{
		append(reinterpret_cast<const unsigned char *>(buff_append.data()),
			buff_append.size());
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

	//returns non NULL terminated string, use size() to find out how long it is
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

	/* Special tail_* Functions
	These functions should not normally be used.

	A possible use of these functions is with something like send() or recv()
	that needs to put bytes in the buffer but we're not sure how many.

	tail_start:
		Returns pointer to start of reserved space past used bytes.
	tail_reserve:
		Reserves free space at end of buffer. The tail_resize() function should be
		used to gain access to this space after it's reserved.
	tail_resize:
		Uses the specified number of bytes in the tail.
	tail_size:
		Returns size of unused space at end of buffer.
	*/
	unsigned char * tail_start()
	{
		assert(reserved > bytes);
		return buff + bytes;
	}
	void tail_reserve(const size_t size)
	{
		allocate(reserved, bytes + size);
	}
	void tail_resize(const size_t size)
	{
		allocate(bytes, bytes + size);
	}
	size_t tail_size()
	{
		assert(reserved > bytes);
		return reserved - bytes;
	}

	//returns size of buffer
	size_t size()
	{
		return bytes;
	}

	/*
	Returns std::string of buffer. This is useful for text protocols.
	*/
	std::string str(const size_t index = 0, const size_t len = npos)
	{
		if(len == npos){
			assert(index < bytes);
			return std::string(reinterpret_cast<const char *>(buff + index), bytes);
		}else{
			assert(index + len <= bytes);
			return std::string(reinterpret_cast<const char *>(buff + index), len);
		}
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
			//requested allocation is equal to what's already allocated
			return;
		}else{
			var = size;
			if(reserved == 0 && bytes == 0){
				//no reserve, nothing in buffer, set buff = NULL to signify empty
				if(buff != NULL){
					//free any memory currently in buffer
					free(buff);
					buff = NULL;
				}
			}else if(buff == NULL){
				//requested allocation non-zero and currently nothing is allocated
				buff = static_cast<unsigned char *>(std::malloc(var));
				assert(buff);
			}else if(var >= reserved){
				/*
				Either reserve needs to be increased, or bytes are above what is
				reserved so we need to allocate beyond what is reserved.
				*/
				buff = static_cast<unsigned char *>(std::realloc(buff, var));
				assert(buff);
			}
		}
	}

	size_t reserved;      //minimum bytes to be left allocated
	size_t bytes;         //how many bytes are currently allocated to buff
	unsigned char * buff; //holds the string
};
}//end of network namespace
#endif
