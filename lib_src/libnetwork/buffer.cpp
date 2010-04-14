#include <network/network.hpp>

//BEGIN iterator
network::buffer::iterator::iterator(
	const int pos_in,
	unsigned char * buf_in
):
	pos(pos_in),
	buf(buf_in)
{

}

network::buffer::iterator::iterator()
{

}

network::buffer::iterator & network::buffer::iterator::operator = (
	const iterator & rval)
{
	pos = rval.pos;
	buf = rval.buf;
	return *this;
}

bool network::buffer::iterator::operator == (const iterator & rval) const
{
	return pos == rval.pos;
}

bool network::buffer::iterator::operator != (const iterator & rval) const
{
	return pos != rval.pos;
}

unsigned char & network::buffer::iterator::operator * ()
{
	return buf[pos];
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
	return iterator(pos_tmp, buf);
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
	return iterator(pos_tmp, buf);
}

network::buffer::iterator network::buffer::iterator::operator + (const int rval)
{
	return iterator(pos + rval, buf);
}

network::buffer::iterator network::buffer::iterator::operator + (const iterator & rval)
{
	return iterator(pos + rval.pos, buf);
}

network::buffer::iterator network::buffer::iterator::operator - (const int rval)
{
	return iterator(pos - rval, buf);
}

ptrdiff_t network::buffer::iterator::operator - (const iterator & rval)
{
	return pos - rval.pos;
}

network::buffer::iterator & network::buffer::iterator::operator += (const int rval)
{
	pos += rval;
	return *this;
}

network::buffer::iterator & network::buffer::iterator::operator += (const iterator & rval)
{
	pos += rval.pos;
	return *this;
}

network::buffer::iterator & network::buffer::iterator::operator -= (const int rval)
{
	pos -= rval;
	return *this;
}

network::buffer::iterator & network::buffer::iterator::operator -= (const iterator & rval)
{
	pos -= rval.pos;
	return *this;
}

unsigned char & network::buffer::iterator::operator [] (const int idx)
{
	return buf[pos + idx];
}

bool network::buffer::iterator::operator < (const iterator & rval) const
{
	return pos < rval.pos;
}

bool network::buffer::iterator::operator > (const iterator & rval) const
{
	return pos > rval.pos;
}

bool network::buffer::iterator::operator >= (const iterator & rval) const
{
	return pos >= rval.pos;
}

bool network::buffer::iterator::operator <= (const iterator & rval) const
{
	return pos <= rval.pos;
}
//END iterator

network::buffer::buffer()
{
	ctor_initialize();
}

network::buffer::buffer(const std::string & S)
{
	ctor_initialize();
	append(S);
}

network::buffer::buffer(const unsigned char * buf_append, const int size)
{
	ctor_initialize();
	append(buf_append, size);
}

network::buffer::buffer(const buffer & B)
{
	ctor_initialize();
	*this = B;
}

network::buffer::~buffer()
{
	if(buf != NULL){
		free(buf);
	}
}

network::buffer & network::buffer::append(const unsigned char ch)
{
	allocate(bytes, bytes + 1);
	buf[bytes - 1] = ch;
	return *this;
}

network::buffer & network::buffer::append(const unsigned char * buf_append, const int size)
{
	allocate(bytes, bytes + size);
	std::memcpy(buf + bytes - size, buf_append, size);
	return *this;
}

network::buffer & network::buffer::append(const std::string & buf_append)
{
	append(reinterpret_cast<const unsigned char *>(buf_append.data()),
		buf_append.size());
	return *this;
}

network::buffer & network::buffer::append(const buffer & buf_append)
{
	append(reinterpret_cast<const unsigned char *>(buf_append.data()),
		buf_append.size());
	return *this;
}

network::buffer::iterator network::buffer::begin() const
{
	return iterator(0, buf);
}

void network::buffer::clear()
{
	allocate(bytes, 0);
}

const unsigned char * network::buffer::data() const
{
	return buf;
}

unsigned char * network::buffer::data()
{
	return buf;
}

bool network::buffer::empty() const
{
	return bytes == 0;
}

network::buffer::iterator network::buffer::end() const
{
	return iterator(bytes, buf);
}

void network::buffer::erase(const int index, const int size)
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

void network::buffer::swap(buffer & rval)
{
	//save current lval
	int tmp_reserved = reserved;
	int tmp_bytes = bytes;
	unsigned char * tmp_buf = buf;

	//copy rval to lval
	reserved = rval.reserved;
	bytes = rval.bytes;
	buf = rval.buf;

	//copy old lval to rval
	rval.reserved = tmp_reserved;
	rval.bytes = tmp_bytes;
	rval.buf = tmp_buf;
}

void network::buffer::reserve(const int size)
{
	allocate(reserved, size);
}

void network::buffer::resize(const int size)
{
	allocate(bytes, size);
}

int network::buffer::size() const
{
	return bytes;
}

std::string network::buffer::str(const int index, const int len) const
{
	if(len == npos){
		assert(index <= bytes);
		return std::string(reinterpret_cast<const char *>(buf + index), bytes - index);
	}else{
		assert(index + len <= bytes);
		return std::string(reinterpret_cast<const char *>(buf + index), len);
	}
}

unsigned char * network::buffer::tail_start() const
{
	assert(reserved > bytes);
	return buf + bytes;
}

void network::buffer::tail_reserve(const int size)
{
	allocate(reserved, bytes + size);
}

void network::buffer::tail_resize(const int size)
{
	allocate(bytes, bytes + size);
}

int network::buffer::tail_size() const
{
	assert(reserved >= bytes);
	return reserved - bytes;
}

const unsigned char network::buffer::operator [] (const int index) const
{
	assert(index >= 0);
	assert(index < bytes);
	return buf[index];
}

unsigned char & network::buffer::operator [] (const int index)
{
	assert(index >= 0);
	assert(index < bytes);
	return buf[index];
}

network::buffer & network::buffer::operator = (const buffer & B)
{
	allocate(bytes, B.bytes);
	std::memcpy(buf, B.buf, bytes);
	return *this;
}

network::buffer & network::buffer::operator = (const char * str)
{
	int len = std::strlen(str);
	allocate(bytes, len);
	std::memcpy(buf, str, bytes);
	return *this;
}

bool network::buffer::operator == (const buffer & B) const
{
	if(bytes != B.bytes){
		return false;
	}else{
		return std::memcmp(buf, B.buf, bytes) == 0;
	}
}

bool network::buffer::operator == (const char * str) const
{
	int len = std::strlen(str);
	if(bytes != len){
		return false;
	}else{
		return std::memcmp(buf, str, bytes) == 0;
	}
}

bool network::buffer::operator != (const buffer & B) const
{
	return !(*this == B);
}

bool network::buffer::operator != (const char * str) const
{
	return !(*this == str);
}

bool network::buffer::operator < (const buffer & B) const
{
	if(bytes < B.bytes){
		return true;
	}else if(bytes > B.bytes){
		return false;
	}else{
		return std::memcmp(buf, B.buf, bytes) < 0;
	}
}

bool network::buffer::operator < (const char * str) const
{
	int len = std::strlen(str);
	if(bytes < len){
		return true;
	}else if(bytes > len){
		return false;
	}else{
		return std::memcmp(buf, str, bytes) < 0;
	}
}

void network::buffer::allocate(int & var, const int size)
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
				std::free(buf);
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

void network::buffer::ctor_initialize()
{
	reserved = 0;
	bytes = 0;
	buf = NULL;
}
