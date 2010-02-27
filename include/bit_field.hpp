#ifndef H_BIT_FIELD
#define H_BIT_FIELD

//include
#include <boost/cstdint.hpp>

//standard
#include <cassert>
#include <string>
#include <vector>

class bit_field
{
public:
	bit_field(const boost::uint64_t bits_in = 0):
		bits(bits_in),
		vec(bits % 8 == 0 ? bits / 8 : bits / 8 + 1, '\0')
	{

	}

	bit_field(const bit_field & BF):
		bits(BF.bits),
		vec(BF.vec)
	{

	}

	bit_field(
		const unsigned char * buf,
		const boost::uint64_t size,
		const boost::uint64_t bits_in
	)
	{
		set_buf(buf, size, bits_in);
	}

	//returns size (bytes) of bit_field with specified bits and group_size 
	static boost::uint64_t size_bytes(const boost::uint64_t bits)
	{
		return bits % 8 == 0 ? bits / 8 : bits / 8 + 1;
	}

	/*
	A reference can't be returned to a few bits so this object is returned
	instead. This object emulates an unsigned int and overloads assignment to
	do the special stuff that needs to happen for assignment.
	*/
	class proxy
	{
		friend class bit_field;
	public:
		//for x = b[i];
		operator bool () const
		{
			return BF.get(idx);
		}

		//for b[i] = x;
		proxy & operator = (const bool val)
		{
			BF.set(idx, val);
			return *this;
		}

		//for b[i] = b[j]
		proxy & operator = (const proxy & rval)
		{
			BF.set(idx, rval.BF.get(rval.idx));
			return *this;
		}

	private:
		proxy(
			bit_field & BF_in,
			const boost::uint64_t idx_in
		):
			BF(BF_in),
			idx(idx_in)
		{}

		bit_field & BF;
		const boost::uint64_t idx;
	};

	//returns proxy object (see documentation for proxy)
	proxy operator [](const boost::uint64_t idx)
	{
		assert(idx < bits);
		return proxy(*this, idx);
	}

	//returns true if two bitgroup_sets are equal
	bool operator == (const bit_field & rval) const
	{
		return vec == rval.vec;
	}

	//returns true if two bitgroup_sets are inequal
	bool operator != (const bit_field & rval) const
	{
		return vec != rval.vec;
	}

	//bitwise AND one bitgroup_set with another
	bit_field & operator &= (const bit_field & rval)
	{
		boost::uint64_t smaller_size = vec.size() < rval.vec.size() ? vec.size() : rval.vec.size();
		for(boost::uint64_t x=0; x<smaller_size; ++x){
			vec[x] &= rval.vec[x];
		}
		return *this;
	}

	//bitwise XOR one bitgroup_set with another
	bit_field & operator ^= (const bit_field & rval)
	{
		boost::uint64_t smaller_size = vec.size() < rval.vec.size() ? vec.size() : rval.vec.size();
		for(boost::uint64_t x=0; x<smaller_size; ++x){
			vec[x] ^= rval.vec[x];
		}
		return *this;
	}

	//bitwise OR one bitgroup_set with another
	bit_field & operator |= (const bit_field & rval)
	{
		boost::uint64_t smaller_size = vec.size() < rval.vec.size() ? vec.size() : rval.vec.size();
		for(boost::uint64_t x=0; x<smaller_size; ++x){
			vec[x] |= rval.vec[x];
		}
		return *this;
	}

	//bitwise NOT bitgroup_set
	bit_field & operator ~ ()
	{
		for(boost::uint64_t x=0; x<vec.size(); ++x){
			vec[x] = ~vec[x];
		}
		return *this;
	}

	//assign a bitgroup_set
	bit_field & operator = (const bit_field & rval)
	{
		bits = rval.bits;
		vec = rval.vec;
		return *this;
	}

	//returns true if all bits set to one
	bool all_set() const
	{
		for(boost::uint64_t x=0; x<bits; ++x){
			if(get(x) != true){
				return false;
			}
		}
		return true;
	}

	//return bit_field as big-endian string
	std::string get_buf() const
	{
		if(vec.empty()){
			return std::string();
		}
		std::string tmp;
		tmp.resize(vec.size());
		for(int x=0, y=vec.size()-1; x<vec.size(); ++x, --y){
			tmp[x] = vec[y];
		}
		return tmp;
	}

	//set size of the bit_field to zero
	void clear()
	{
		bits = 0;
		vec.clear();
	}

	//returns true if the bit_field holds zero elements
	bool empty() const
	{
		return bits == 0;
	}

	//return true if all bits set to zero
	bool none_set() const
	{
		for(boost::uint64_t x=0; x<bits; ++x){
			if(get(x) != false){
				return false;
			}
		}
		return true;
	}

	//set all bits to zero
	void reset()
	{
		for(boost::uint64_t x=0; x<vec.size(); ++x){
			vec[x] = 0;
		}
	}

	//change size of bit_field
	void resize(const boost::uint64_t bits_in)
	{
		bits = bits_in;
		vec.resize(bits % 8 == 0 ? bits / 8 : bits / 8 + 1, '\0');
	}

	//set all bits
	void set()
	{
		for(boost::uint64_t x=0; x<vec.size(); ++x){
			vec[x] = 255;
		}
	}

	//set internal buffer from big-endian source buf
	void set_buf(const unsigned char * buf, const boost::uint64_t size,
		const boost::uint64_t bits_in)
	{
		assert(size_bytes(bits_in) == size);
		bits = bits_in;
		vec.resize(size);
		for(boost::uint64_t x=0, y=vec.size()-1; x<vec.size(); ++x, --y){
			vec[x] = buf[y];
		}
	}

	//returns the number of bitbits in the bitgroup_set
	boost::uint64_t size() const
	{
		return bits;
	}

private:
	boost::uint64_t bits;
	std::vector<unsigned char> vec;

	//get a number from the bitgroup_set
	bool get(const boost::uint64_t idx) const
	{
		return vec[idx / 8] & (1 << idx % 8);
	}

	//set a number in the bitgroup_set
	void set(const boost::uint64_t idx, const bool val)
	{
		if(val){
			vec[idx / 8] |= (1 << idx % 8);
		}else{
			vec[idx / 8] &= ~(1 << idx % 8);
		}
	}
};
#endif
