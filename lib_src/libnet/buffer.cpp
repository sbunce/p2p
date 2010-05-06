#include <network/network.hpp>

//BEGIN iterator
network::buffer::iterator::iterator(
	const unsigned pos_in,
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
	unsigned pos_tmp = pos;
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
	unsigned pos_tmp = pos;
	--pos;
	return iterator(pos_tmp, buf);
}

network::buffer::iterator network::buffer::iterator::operator + (const unsigned rval)
{
	return iterator(pos + rval, buf);
}

network::buffer::iterator network::buffer::iterator::operator + (const iterator & rval)
{
	return iterator(pos + rval.pos, buf);
}

network::buffer::iterator network::buffer::iterator::operator - (const unsigned rval)
{
	return iterator(pos - rval, buf);
}

ptrdiff_t network::buffer::iterator::operator - (const iterator & rval)
{
	return pos - rval.pos;
}

network::buffer::iterator & network::buffer::iterator::operator += (const unsigned rval)
{
	pos += rval;
	return *this;
}

network::buffer::iterator & network::buffer::iterator::operator += (const iterator & rval)
{
	pos += rval.pos;
	return *this;
}

network::buffer::iterator & network::buffer::iterator::operator -= (const unsigned rval)
{
	pos -= rval;
	return *this;
}

network::buffer::iterator & network::buffer::iterator::operator -= (const iterator & rval)
{
	pos -= rval.pos;
	return *this;
}

unsigned char & network::buffer::iterator::operator [] (const unsigned idx)
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

network::buffer::buffer(const unsigned char * buf_append, const unsigned size)
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

network::buffer & network::buffer::append(const unsigned char * buf_append, const unsigned size)
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

void network::buffer::erase(const unsigned idx)
{
	assert(idx < bytes);
	allocate(bytes, idx);
}

void network::buffer::erase(const unsigned idx, const unsigned size)
{
	assert(idx + size <= bytes);
	std::memmove(buf + idx, buf + idx + size, bytes - idx - size);
	allocate(bytes, bytes - size);
}

void network::buffer::swap(buffer & rval)
{
	//save current lval
	unsigned tmp_reserved = reserved;
	unsigned tmp_bytes = bytes;
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

void network::buffer::reserve(const unsigned size)
{
	allocate(reserved, size);
}

void network::buffer::resize(const unsigned size)
{
	allocate(bytes, size);
}

unsigned network::buffer::size() const
{
	return bytes;
}

std::string network::buffer::str() const
{
	return std::string(reinterpret_cast<const char *>(buf), bytes);
}

std::string network::buffer::str(const unsigned idx) const
{
	assert(idx <= bytes);
	return std::string(reinterpret_cast<const char *>(buf + idx), bytes - idx);
}

std::string network::buffer::str(const unsigned idx, const unsigned len) const
{
	assert(idx + len <= bytes);
	return std::string(reinterpret_cast<const char *>(buf + idx), len);
}

unsigned char * network::buffer::tail_start() const
{
	assert(reserved > bytes);
	return buf + bytes;
}

void network::buffer::tail_reserve(const unsigned size)
{
	allocate(reserved, bytes + size);
}

void network::buffer::tail_resize(const unsigned size)
{
	allocate(bytes, bytes + size);
}

unsigned network::buffer::tail_size() const
{
	assert(reserved >= bytes);
	return reserved - bytes;
}

const unsigned char network::buffer::operator [] (const unsigned idx) const
{
	assert(idx < bytes);
	return buf[idx];
}

unsigned char & network::buffer::operator [] (const unsigned idx)
{
	assert(idx < bytes);
	return buf[idx];
}

network::buffer & network::buffer::operator = (const buffer & B)
{
	allocate(bytes, B.bytes);
	std::memcpy(buf, B.buf, bytes);
	return *this;
}

network::buffer & network::buffer::operator = (const char * str)
{
	unsigned len = std::strlen(str);
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
	unsigned len = std::strlen(str);
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
	unsigned len = std::strlen(str);
	if(bytes < len){
		return true;
	}else if(bytes > len){
		return false;
	}else{
		return std::memcmp(buf, str, bytes) < 0;
	}
}

void network::buffer::allocate(unsigned & var, const unsigned size)
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
