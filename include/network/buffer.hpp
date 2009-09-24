/*
This container is modeled after a std::string.

This container doesn't do copy-on-write optimization which can make writing
thread safe code difficult. This container also exposes it's underlying buffer
in a non-const way which makes it possible to use it with send()/recv() without
extra copying.

This container offers many convenience functions for encoding data to be sent
over a network.
*/
#ifndef H_NETWORK_BUFFER
#define H_NETWORK_BUFFER

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
	//returned to indicate <something> not found
	static const size_t npos = -1;

	buffer();
	buffer(const buffer & Buffer);
	~buffer();

	class iterator : public std::iterator<std::random_access_iterator_tag, unsigned char>
	{
		friend class buffer;
	public:
		iterator();
		iterator & operator = (const iterator & rval);
		bool operator == (const iterator & rval) const;
		bool operator != (const iterator & rval) const;
		unsigned char & operator * ();
		//operator -> doesn't have to be defined because ->m is not well defined
		iterator & operator ++ ();
		iterator operator ++ (int);
		iterator & operator -- ();
		iterator operator -- (int);
		iterator operator + (const int rval);
		iterator operator + (const iterator & rval);
		iterator operator - (const int rval);
		ptrdiff_t operator - (const iterator & rval);
		iterator & operator += (const int rval);
		iterator & operator += (const iterator & rval);
		iterator & operator -= (const int rval);
		iterator & operator -= (const iterator & rval);
		unsigned char & operator [] (const int idx);
		bool operator < (const iterator & rval) const;
		bool operator > (const iterator & rval) const;
		bool operator >= (const iterator & rval) const;
		bool operator <= (const iterator & rval) const;

	private:
		iterator(
			const int pos_in,
			unsigned char * buff_in
		);

		int pos;              //offset from start of buffer iterator is at
		unsigned char * buff; //buff that exists in buffer which spawned this iterator
	};

	/*
	append:
		Appends 1 byte. Appends size bytes from buff_append. Appends a
		std::string.
	begin:
		Returns iterator to first unsigned char of buffer.
	clear:
		Clears contents of buffer.
	data:
		Returns non NULL terminated string, use size() to find out how long it is.
	empty:
		Returns true if buffer size is zero.
	end:
		Returns iterator to one past last unsigned char of buffer.
	erase:
		Erase bytes starting at index, ending at index + length. If no length
		specified bytes erased starting at index to end of buffer.
	reserve:
		Keeps a specified minimum number of bytes allocated.
	resize:
		Makes buffer the specified size, memory in buffer left uninitialized.
	size:
		Returns size of buffer.
	str:
		Returns std::string of buffer. Useful for text protocols.
	tail_start:
		Returns pointer to start of reserved space at end of buffer.
	tail_reserve:
		Reserves free space at end of buffer. The tail_resize() function should be
		used to gain access to this space after it's reserved.
	tail_resize:
		Uses the specified number of bytes in the tail.
	tail_size:
		Returns size of reserved space at end of buffer.
	*/
	buffer & append(const unsigned char char_append);
	buffer & append(const unsigned char * buff_append, const size_t size);
	buffer & append(const std::string & buff_append);
	iterator begin();
	void clear();
	unsigned char * data();
	bool empty();
	iterator end();
	void erase(const size_t index, const size_t length = npos);
	void reserve(const size_t size);
	void resize(const size_t size);
	size_t size();
	std::string str(const size_t index = 0, const size_t len = npos);
	unsigned char * tail_start();
	void tail_reserve(const size_t size);
	void tail_resize(const size_t size);
	size_t tail_size();

	//operators
	unsigned char & operator [] (const size_t index);
	bool operator == (const buffer & Buffer);
	bool operator != (const buffer & Buffer);
	bool operator == (const std::string & str);
	bool operator != (const std::string & str);

private:
	size_t reserved;      //minimum bytes to be left allocated
	size_t bytes;         //how many bytes are currently allocated to buff
	unsigned char * buff; //holds the string

	/*
	allocate:
		Used for both reserve and normal allocation. When using to reserve pass in
		reserved for var parameter. When using to allocate normally pass in bytes
		for var parameter. The size parameter is the desired total size to
		reserve, or allocate normally, which depends on how this function is being
		used.
	*/
	void allocate(size_t & var, const size_t & size);
};
}//end of network namespace
#endif
