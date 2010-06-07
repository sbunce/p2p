#ifndef H_BIT_FIELD
#define H_BIT_FIELD

//include
#include <boost/cstdint.hpp>

//standard
#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

class bit_field
{
public:
	bit_field(const boost::uint64_t bits_in = 0):
		bits(bits_in),
		vec(bits % 8 == 0 ? bits / 8 : bits / 8 + 1, '\0'),
		bits_set(0)
	{

	}

	bit_field(const bit_field & BF):
		bits(BF.bits),
		vec(BF.vec),
		bits_set(BF.bits_set)
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
			const bit_field & BF_in,
			const boost::uint64_t idx_in
		):
			//const_cast needed for const operator []
			BF(const_cast<bit_field &>(BF_in)),
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

	//const version of above
	const proxy operator [](const boost::uint64_t idx) const
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
		calc_bits_set();
		return *this;
	}

	//bitwise XOR one bitgroup_set with another
	bit_field & operator ^= (const bit_field & rval)
	{
		boost::uint64_t smaller_size = vec.size() < rval.vec.size() ? vec.size() : rval.vec.size();
		for(boost::uint64_t x=0; x<smaller_size; ++x){
			vec[x] ^= rval.vec[x];
		}
		calc_bits_set();
		return *this;
	}

	//bitwise OR one bitgroup_set with another
	bit_field & operator |= (const bit_field & rval)
	{
		boost::uint64_t smaller_size = vec.size() < rval.vec.size() ? vec.size() : rval.vec.size();
		for(boost::uint64_t x=0; x<smaller_size; ++x){
			vec[x] |= rval.vec[x];
		}
		calc_bits_set();
		return *this;
	}

	//bitwise NOT bitgroup_set
	bit_field & operator ~ ()
	{
		for(boost::uint64_t x=0; x<vec.size(); ++x){
			vec[x] = ~vec[x];
		}
		calc_bits_set();
		return *this;
	}

	//assign a bitgroup_set
	bit_field & operator = (const bit_field & rval)
	{
		bits = rval.bits;
		vec = rval.vec;
		bits_set = rval.bits_set;
		return *this;
	}

	//returns true if all bits set to one
	bool all_set() const
	{
		return bits == bits_set;
	}

	//return bit_field as big-endian string
	std::string get_buf() const
	{
		std::string tmp;
		tmp.resize(vec.size());
		std::reverse_copy(vec.begin(), vec.end(), tmp.begin());
		return tmp;
	}

	//set size of the bit_field to zero
	void clear()
	{
		bits = 0;
		vec.clear();
		bits_set = 0;
	}

	//returns true if the bit_field holds zero elements
	bool empty() const
	{
		return bits == 0;
	}

	//return true if all bits set to zero
	bool none_set() const
	{
		return bits_set == 0;
	}

	//set all bits to zero
	void reset()
	{
		for(boost::uint64_t x=0; x<vec.size(); ++x){
			vec[x] = 0;
		}
		bits_set = 0;
	}

	//change size of bit_field
	void resize(const boost::uint64_t bits_in)
	{
		bits = bits_in;
		vec.resize(bits % 8 == 0 ? bits / 8 : bits / 8 + 1, '\0');
		calc_bits_set();
	}

	//set all bits to true
	void set()
	{
		for(boost::uint64_t x=0; x<vec.size(); ++x){
			vec[x] = 255;
		}
		bits_set = bits;
	}

	//set internal buffer from big-endian source buf
	void set_buf(const unsigned char * buf, const boost::uint64_t size,
		const boost::uint64_t bits_in)
	{
		assert(size_bytes(bits_in) == size);
		bits = bits_in;
		vec.resize(size);
		std::reverse_copy(buf, buf+size, vec.begin());
		calc_bits_set();
	}

	//returns number of bits set to 1
	boost::uint64_t set_count() const
	{
		return bits_set;
	}

	//returns the number of bitbits in the bitgroup_set
	boost::uint64_t size() const
	{
		return bits;
	}

private:
	boost::uint64_t bits;           //number of bits in bit_field
	std::vector<unsigned char> vec; //internal buffer
	boost::uint64_t bits_set;       //number of bits set to 1

	//recalculates bits_set
	void calc_bits_set()
	{
		bits_set = 0;
		for(boost::uint64_t x=0; x<bits; ++x){
			bits_set += get(x);
		}
	}

	//get a number from the bitgroup_set
	bool get(const boost::uint64_t idx) const
	{
		return vec[idx / 8] & (1 << idx % 8);
	}

	//set a number in the bitgroup_set
	void set(const boost::uint64_t idx, const bool val)
	{
		if(val){
			if(get(idx) == false){
				++bits_set;
			}
			vec[idx / 8] |= (1 << idx % 8);
		}else{
			if(get(idx) == true){
				--bits_set;
			}
			vec[idx / 8] &= ~(1 << idx % 8);
		}
	}
};
#endif
