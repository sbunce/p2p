#ifndef H_NET_BUFFER
#define H_NET_BUFFER

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

namespace net{
class buffer
{
public:
	buffer();
	buffer(const std::string & S);
	buffer(const unsigned char * buf_append, const unsigned size);
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
		iterator operator + (const unsigned rval);
		iterator operator + (const iterator & rval);
		iterator operator - (const unsigned rval);
		ptrdiff_t operator - (const iterator & rval);
		iterator & operator += (const unsigned rval);
		iterator & operator += (const iterator & rval);
		iterator & operator -= (const unsigned rval);
		iterator & operator -= (const iterator & rval);
		unsigned char & operator [] (const unsigned idx);
		bool operator < (const iterator & rval) const;
		bool operator > (const iterator & rval) const;
		bool operator >= (const iterator & rval) const;
		bool operator <= (const iterator & rval) const;

	private:
		iterator(
			const unsigned pos_in,
			unsigned char * buf_in
		);

		unsigned pos;        //offset from start of buffer iterator is at
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
		Erase bytes in buffer starting at idx and size chars long.
	swap:
		Swap one buffer with another without copying internal buffer.
	reserve:
		Specified amount of memory will be left allocated.
	resize:
		Change the size of the buffer.
	size:
		Returns size (bytes) of buffer.
	str (no parameters):
		Returns std::string of buffer.
	str (one parameters):
		Returns std::string of buffer from idx to end.
	str (two parameters):
		Returns std::string of buffer starting at idx and len elements long.
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
	buffer & append(const unsigned char * buf_append, const unsigned size);
	buffer & append(const std::string & buf_append);
	buffer & append(const buffer & buf_append);
	iterator begin() const;
	void clear();
	const unsigned char * data() const;
	unsigned char * data();
	bool empty() const;
	iterator end() const;
	void erase(const unsigned idx);
	void erase(const unsigned idx, const unsigned size);
	void swap(buffer & rval);
	void reserve(const unsigned size);
	void resize(const unsigned size);
	unsigned size() const;
	std::string str() const;
	std::string str(const unsigned idx) const;
	std::string str(const unsigned idx, const unsigned len) const;
	unsigned char * tail_start() const;
	void tail_reserve(const unsigned size);
	void tail_resize(const unsigned size);
	unsigned tail_size() const;

	//operators
	const unsigned char operator [] (const unsigned idx) const;
	unsigned char & operator [] (const unsigned idx);
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
	unsigned reserved;   //minimum bytes to be left allocated
	unsigned bytes;      //how many bytes are currently allocated to buf
	unsigned char * buf; //what the buffer stores

	/*
	allocate:
		Resizes or reserves the buffer. To resize pass in bytes for the var
		parameter. To reserve pass in reserved for the var parameter. The size
		parameter is the size to resize to or reserve.
	ctor_initialize:
		Called by all ctor's to initialize data members.
	*/
	void allocate(unsigned & var, const unsigned size);
	void ctor_initialize();
};
}//end of namespace net
#endif
