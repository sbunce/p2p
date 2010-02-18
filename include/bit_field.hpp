#ifndef H_BIT_FIELD
#define H_BIT_FIELD

//include
#include <logger.hpp>

//standard
#include <algorithm>
#include <cmath>
#include <vector>

class bit_field
{
public:
	bit_field(
		const boost::uint64_t groups_in = 0,
		const unsigned group_size_in = 1
	):
		groups(groups_in),
		group_size(group_size_in),
		vec((groups * group_size) % 8 == 0 ?
			((groups * group_size) / 8) : ((groups * group_size) / 8 + 1),
			0
		),
		max_group_value(std::pow(static_cast<const double>(2),
			static_cast<const int>(group_size)) - 1)
	{
		if(group_size < 1 && group_size > 7){
			LOGGER << "group_size must be greater than 0 and less than 8";
			exit(1);
		}
	}

	bit_field(
		const bit_field & BF
	):
		groups(BF.groups),
		group_size(BF.group_size),
		vec(BF.vec),
		max_group_value(BF.max_group_value)
	{

	}

	bit_field(
		const unsigned char * buf,
		const unsigned size,
		const boost::uint64_t groups_in,
		const boost::uint64_t group_size_in
	)
	{
		set_buf(buf, size, groups_in, group_size_in);
	}

	static const boost::uint64_t npos = -1;

	//returns size (bytes) of bit_field with specified groups and group_size 
	static boost::uint64_t size_bytes(boost::uint64_t groups, boost::uint64_t group_size)
	{
		boost::uint64_t tmp = groups * group_size;
		if(tmp % 8 == 0){
			return tmp / 8;
		}else{
			return tmp / 8 + 1;
		}
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
		operator boost::uint64_t() const
		{
			return BF.get_num(index);
		}

		//for b[i] = x;
		proxy & operator = (const boost::uint64_t num)
		{
			BF.set_num(index, num);
			return *this;
		}

		//for b[i] = b[j]
		proxy & operator = (const proxy & rval)
		{
			BF.set_num(index, rval.BF.get_num(rval.index));
			return *this;
		}

	private:
		proxy(
			bit_field & BF_in,
			const boost::uint64_t index_in
		):
			BF(BF_in),
			index(index_in)
		{}

		bit_field & BF;
		const boost::uint64_t index;
	};

	//returns proxy object (see documentation for proxy)
	proxy operator [](const boost::uint64_t index)
	{
		assert(index < groups);
		return proxy(*this, index);
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
		assert(group_size == rval.group_size);
		groups = rval.groups;
		vec = rval.vec;
		resize(groups);
		return *this;
	}

	//returns true if all bits set to one
	bool all_set() const
	{
		for(boost::uint64_t x=0; x<groups; ++x){
			if(get_num(x) != max_group_value){
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

	//sets the size of the bit_field to zero
	void clear()
	{
		groups = 0;
		vec.clear();
	}

	//returns true if the bit_field holds zero elements
	bool empty() const
	{
		return groups == 0;
	}

	//return true if all bits set to zero
	bool none_set() const
	{
		for(boost::uint64_t x=0; x<groups; ++x){
			if(get_num(x) != 0){
				return false;
			}
		}
		return true;
	}

	/*
	Sets all bitgroups to zero. The way this function works is faster than
	looping through the individual bitgroups and setting them to zero.
	*/
	void reset()
	{
		for(boost::uint64_t x=0; x<vec.size(); ++x){
			vec[x] = 0;
		}
	}

	//changes the number of groups in the bit_field to specified size
	void resize(const boost::uint64_t size)
	{
		groups = size;
		vec.resize(
			(groups * group_size) % 8 == 0 ?
			((groups * group_size) / 8) : ((groups * group_size) / 8 + 1),
			'\0'
		);
	}

	//sets all groups to their max value (sets all bits to 1)
	void set()
	{
		for(boost::uint64_t x=0; x<groups; ++x){
			set_num(x, max_group_value);
		}
	}

	//set internal buffer from big-endian encoded buf
	void set_buf(const unsigned char * buf, const unsigned size,
		const boost::uint64_t groups_in, const boost::uint64_t group_size_in)
	{
		groups = groups_in;
		group_size = group_size_in;
		max_group_value = std::pow(static_cast<const double>(2),
			static_cast<const int>(group_size)) - 1;
		assert(size_bytes(groups, group_size) == size);
		vec.resize(size);
		for(boost::uint64_t x=0, y=vec.size()-1; x<vec.size(); ++x, --y){
			vec[x] = buf[y];
		}
	}

	//returns the number of bitgroups in the bitgroup_set
	unsigned size() const
	{
		return groups;
	}

private:
	boost::uint64_t groups;    //how many bit groups
	unsigned group_size; //how many bits in each group

	/*
	The bitgroups are stored here.
	Note: When the group_size > 1 bitgroups can partially span two bytes.
	*/
	std::vector<unsigned char> vec;

	/*
	Maximum value a bitgroup can hold.
		2^group_size - 1
	*/
	unsigned max_group_value;

	//get a number from the bitgroup_set
	unsigned get_num(const boost::uint64_t index) const
	{
		boost::uint64_t byte_offset = index * group_size / 8;
		boost::uint64_t bit_offset = index * group_size % 8;
		unsigned char temp, result = 0;
		if(bit_offset + group_size < 8){
			//bits on left don't belong to this bitgroup
			temp = vec[byte_offset];
			temp <<= 8 - bit_offset - group_size;
			temp >>= 8 - group_size;
			result |= temp;
		}else{
			//bits on left all belong to this bitgroup
			temp = vec[byte_offset];
			temp >>= bit_offset;
			result |= temp;
			if(bit_offset + group_size != 8){
				//bitgroup spans two bytes
				temp = vec[byte_offset+1];
				temp <<= 16 - bit_offset - group_size;
				temp >>= 16 - bit_offset - group_size;
				temp <<= -bit_offset + 8;
				result |= temp;
			}
		}
		return result;
	}

	//set a number in the bitgroup_set
	void set_num(const boost::uint64_t index, const unsigned num)
	{
		assert(num <= max_group_value);
		boost::uint64_t byte_offset = index * group_size / 8;
		boost::uint64_t bit_offset = index * group_size % 8;
		unsigned char temp;

		//use bitmask to clear group bits
		temp = max_group_value;
		temp <<= bit_offset;
		temp = ~temp;
		vec[byte_offset] &= temp;

		//write bits in first byte
		temp = num;
		temp <<= bit_offset;
		vec[byte_offset] |= temp;

		//if true then bitgroup spans two bytes
		if(bit_offset + group_size > 8){
			//clear bits on the right by doing bit shifts
			vec[byte_offset+1] >>= 8 - bit_offset + group_size;
			vec[byte_offset+1] <<= 8 - bit_offset + group_size;

			//write bits in second byte
			temp = num;
			temp >>= -bit_offset + 8;
			vec[byte_offset+1] |= temp;
		}
	}
};
#endif
