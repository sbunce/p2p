#include <network/network.hpp>

//BEGIN iterator implementation
network::buffer::iterator::iterator()
{

}

network::buffer::iterator::iterator(
	const int pos_in,
	unsigned char * buff_in
):
	pos(pos_in),
	buff(buff_in)
{

}

network::buffer::iterator & network::buffer::iterator::operator = (
	const network::buffer::iterator & rval)
{
	pos = rval.pos;
	buff = rval.buff;
	return *this;
}
bool network::buffer::iterator::operator == (
	const network::buffer::iterator & rval) const
{
	return pos == rval.pos;
}
bool network::buffer::iterator::operator != (
	const network::buffer::iterator & rval) const
{
	return pos != rval.pos;
}
unsigned char & network::buffer::iterator::operator * ()
{
	return buff[pos];
}

network::buffer::iterator & network::buffer::iterator::operator ++ ()
{
	++pos;
	return *this;
}

network::buffer::iterator network::buffer::iterator::operator ++ (int)
{
	int pos_tmp = pos;
	++pos;
	return iterator(pos_tmp, buff);
}

network::buffer::iterator & network::buffer::iterator::operator -- ()
{
	--pos;
	return *this;
}

network::buffer::iterator network::buffer::iterator::operator -- (int)
{
	int pos_tmp = pos;
	--pos;
	return iterator(pos_tmp, buff);
}

network::buffer::iterator network::buffer::iterator::operator + (const int rval)
{
	return iterator(pos + rval, buff);
}

network::buffer::iterator network::buffer::iterator::operator + (
	const network::buffer::iterator & rval)
{
	return iterator(pos + rval.pos, buff);
}

network::buffer::iterator network::buffer::iterator::operator - (const int rval)
{
	return iterator(pos - rval, buff);
}

ptrdiff_t network::buffer::iterator::operator - (
	const network::buffer::iterator & rval)
{
	return pos - rval.pos;
}

network::buffer::iterator & network::buffer::iterator::operator += (const int rval)
{
	pos += rval;
	return *this;
}

network::buffer::iterator & network::buffer::iterator::operator += (
	const network::buffer::iterator & rval)
{
	pos += rval.pos;
	return *this;
}

network::buffer::iterator & network::buffer::iterator::operator -= (const int rval)
{
	pos -= rval;
	return *this;
}

network::buffer::iterator & network::buffer::iterator::operator -= (
	const network::buffer::iterator & rval)
{
	pos -= rval.pos;
	return *this;
}

unsigned char & network::buffer::iterator::operator [] (const int idx)
{
	return buff[pos + idx];
}

bool network::buffer::iterator::operator < (
	const network::buffer::iterator & rval) const
{
	return pos < rval.pos;
}

bool network::buffer::iterator::operator > (
	const network::buffer::iterator & rval) const
{
	return pos > rval.pos;
}

bool network::buffer::iterator::operator >= (
	const network::buffer::iterator & rval) const
{
	return pos >= rval.pos;
}

bool network::buffer::iterator::operator <= (
	const network::buffer::iterator & rval) const
{
	return pos <= rval.pos;
}
//END iterator implementation

network::buffer::buffer():
	reserved(0),
	bytes(0),
	buff(NULL)
{

}

network::buffer::buffer(const buffer & Buffer):
	reserved(Buffer.reserved),
	bytes(Buffer.bytes)
{
	buff = static_cast<unsigned char *>(std::malloc(bytes));
	std::memcpy(buff, Buffer.buff, bytes);
}

network::buffer::~buffer()
{
	if(buff != NULL){
		free(buff);
	}
}

network::buffer & network::buffer::append(const unsigned char char_append)
{
	allocate(bytes, bytes + 1);
	buff[bytes - 1] = char_append;
	return *this;
}

network::buffer & network::buffer::append(const unsigned char * buff_append,
	const size_t size)
{
	allocate(bytes, bytes + size);
	std::memcpy(buff + bytes - size, buff_append, size);
	return *this;
}

network::buffer & network::buffer::append(const std::string & buff_append)
{
	append(reinterpret_cast<const unsigned char *>(buff_append.data()),
		buff_append.size());
	return *this;
}

void network::buffer::allocate(size_t & var, const size_t & size)
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

network::buffer::iterator network::buffer::begin()
{
	return iterator(0, buff);
}

void network::buffer::clear()
{
	allocate(bytes, 0);
}

unsigned char * network::buffer::data()
{
	return buff;
}

bool network::buffer::empty()
{
	return bytes == 0;
}

network::buffer::iterator network::buffer::end()
{
	return iterator(bytes, buff);
}

void network::buffer::erase(const size_t index, const size_t length)
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

void network::buffer::reserve(const size_t size)
{
	allocate(reserved, size);
}

void network::buffer::resize(const size_t size)
{
	allocate(bytes, size);
}

size_t network::buffer::size()
{
	return bytes;
}

std::string network::buffer::str(const size_t index, const size_t len)
{
	if(len == npos){
		assert(index < bytes);
		return std::string(reinterpret_cast<const char *>(buff + index), bytes);
	}else{
		assert(index + len <= bytes);
		return std::string(reinterpret_cast<const char *>(buff + index), len);
	}
}

unsigned char * network::buffer::tail_start()
{
	assert(reserved > bytes);
	return buff + bytes;
}

void network::buffer::tail_reserve(const size_t size)
{
	allocate(reserved, bytes + size);
}

void network::buffer::tail_resize(const size_t size)
{
	allocate(bytes, bytes + size);
}

size_t network::buffer::tail_size()
{
	assert(reserved > bytes);
	return reserved - bytes;
}

unsigned char & network::buffer::operator [] (const size_t index)
{
	assert(index < bytes);
	return buff[index];
}

bool network::buffer::operator == (const buffer & Buffer)
{
	if(bytes != Buffer.bytes){
		return false;
	}else{
		return std::memcmp(buff, Buffer.buff, bytes) == 0;
	}
}

bool network::buffer::operator != (const buffer & Buffer)
{
	return !(*this == Buffer);
}

bool network::buffer::operator == (const std::string & str)
{
	if(bytes != str.size()){
		return false;
	}else{
		return std::memcmp(buff, str.data(), bytes) == 0;
	}
}

bool network::buffer::operator != (const std::string & str)
{
	return !(*this == str);
}
