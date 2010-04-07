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
	static const int npos = -1;

	buffer();
	buffer(const std::string & S);
	buffer(const unsigned char * buf_append, const int size);
	buffer(const buffer & B);
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
			unsigned char * buf_in
		);

		int pos;             //offset from start of buffer iterator is at
		unsigned char * buf; //buf that exists in buffer which spawned this iterator
	};

	/*
	append:
		Many different functions to append various things.
	begin:
		Return iterator to beginning of buffer.
	clear:
		Clears buffer.
	data:
		Returns pointer to internal buffer. Both const and non const versions
		available.
	empty:
		Returns true if buffer empty.
	end:
		Return iterator to end of buffer.
	erase:
		Erase bytes in buffer starting at index and size chars long.
	swap:
		Swap one buffer with another without copying internal buffer.
	reserve:
		Specified amount of memory will be left allocated.
	resize:
		Change the size of the buffer.
	size:
		Returns size (bytes) of buffer.
	str:
		Returns std::string of buffer starting at index and len chars long.
	tail_start:
		Returns pointer to start of unused space at end of buffer.
	tail_reserve:
		Reserve space at end of buffer.
	tail_resize:
		Use space at end of buffer.
	tail_size:
		Size of unused space at end of buffer.
	*/
	buffer & append(const unsigned char ch);
	buffer & append(const unsigned char * buf_append, const int size);
	buffer & append(const std::string & buf_append);
	buffer & append(const buffer & buf_append);
	iterator begin() const;
	void clear();
	const unsigned char * data() const;
	unsigned char * data();
	bool empty() const;
	iterator end() const;
	void erase(const int index, const int size = npos);
	void swap(buffer & rval);
	void reserve(const int size);
	void resize(const int size);
	int size() const;
	std::string str(const int index = 0, const int len = npos) const;
	unsigned char * tail_start() const;
	void tail_reserve(const int size);
	void tail_resize(const int size);
	int tail_size() const;

	//operators
	const unsigned char operator [] (const int index) const;
	unsigned char & operator [] (const int index);
	buffer & operator = (const buffer & B);
	buffer & operator = (const char * str);
	bool operator == (const buffer & B) const;
	bool operator == (const char * str) const;
	bool operator != (const buffer & B) const;
	bool operator != (const char * str) const;
	bool operator < (const buffer & B) const;
	bool operator < (const char * str) const;

	friend std::ostream & operator << (std::ostream & lval, const buffer & rval)
	{
		return lval << rval.str();
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
	ctor_initialize:
		Called by all ctor's to initialize data members.
	*/
	void allocate(int & var, const int size);
	void ctor_initialize();
};
}//end of network namespace
#endif
