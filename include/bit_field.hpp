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
		const boost::uint64_t & groups_in,
		const unsigned group_size_in = 1
	):
		groups(groups_in),
		group_size(group_size_in),
		vec((groups * group_size) % 8 == 0 ?
			((groups * group_size) / 8) : ((groups * group_size) / 8 + 1),
			'\0'
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

	static const boost::uint64_t npos = -1;

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
		proxy & operator = (const boost::uint64_t & num)
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
			const boost::uint64_t & index_in
		):
			BF(BF_in),
			index(index_in)
		{}

		bit_field & BF;
		const boost::uint64_t index;
	};

	//returns proxy object (see documentation for proxy)
	proxy operator [](const boost::uint64_t & index)
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

	/*
	Searches the bit_field starting at 0 for an element != 0. Returns the index
	of that element or npos if all bits are 0.
	*/
	boost::uint64_t find_first() const
	{
		if(groups == 0){
			return npos;
		}else{
			for(boost::uint64_t x=0; x<groups; ++x){
				if(get_num(x) != 0){
					return x;
				}
			}
			return npos;
		}
	}

	/*
	Searches the bit_field starting at > pos for an element != 0. Returns the index
	of that element or npos if all bits are 0.
	Note: If size() == 0 then npos is always returned.
	*/
	boost::uint64_t find_next(const boost::uint64_t & pos) const
	{
		if(groups == 0){
			return npos;
		}else{
			for(boost::uint64_t x=pos+1; x<groups; ++x){
				if(get_num(x) != 0){
					return x;
				}
			}
			return npos;
		}
	}

	//returns const reference to internal buffer
	const std::vector<unsigned char> & internal_buffer() const
	{
		return vec;
	}

	//return true if all groups are equal to zero
	bool none() const
	{
		for(boost::uint64_t x=0; x<vec.size(); ++x){
			if(vec[x] != 0){
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
	void resize(const boost::uint64_t & size)
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
		for(boost::uint64_t x=0; x<vec.size(); ++x){
			vec[x] = 255;
		}
	}

	//returns the number of bitgroups in the bitgroup_set
	unsigned size() const
	{
		return groups;
	}

private:
	boost::uint64_t groups;    //how many bit groups
	const unsigned group_size; //how many bits in each group

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
	boost::uint64_t get_num(const boost::uint64_t & index) const
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
	void set_num(const boost::uint64_t & index, const unsigned num)
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
