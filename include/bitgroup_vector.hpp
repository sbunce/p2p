#ifndef H_BITGROUP_VECTOR
#define H_BITGROUP_VECTOR

//include
#include <logger.hpp>

//standard
#include <cmath>
#include <vector>

class bitgroup_vector
{
public:
	bitgroup_vector(
		const unsigned groups_in,
		const unsigned group_size_in
	):
		groups(groups_in),
		group_size(group_size_in),
		vec((groups * group_size) % 8 == 0 ?
			((groups * group_size) / 8) : ((groups * group_size) / 8 + 1),
			'\0'
		),
		max_group_value(std::pow(2, group_size) - 1)
	{
		if(groups == 0){
			LOGGER << "bitgroup_set only supports groups > 0";
			exit(1);
		}
		if(group_size < 1 && group_size > 7){
			LOGGER << "group_size must be greater than 0 and less than 8";
			exit(1);
		}
	}

	/*
	A reference can't be returned to a few bits so this object is returned
	instead. This object emulates an unsigned int and overloads assignment to
	do that special stuff that needs to happen upon assignment.
	*/
	class proxy
	{
		friend class bitgroup_vector;
	public:
		//for x = b[i];
		operator unsigned() const
		{
			return BGV.get_num(index);
		}

		//for b[i] = x;
		proxy & operator = (const unsigned num)
		{
			BGV.set_num(index, num);
			return *this;
		}

		//for b[i] = b[j]
		proxy & operator = (const proxy & rval)
		{
			BGV.set_num(index, rval.BGV.get_num(rval.index));
			return *this;
		}

	private:
		proxy(
			bitgroup_vector & BGV_in,
			const size_t index_in
		):
			BGV(BGV_in),
			index(index_in)
		{}

		bitgroup_vector & BGV;
		const size_t index;
	};

	//returns proxy object (see documentation for proxy)
	proxy operator [](const size_t index)
	{
		assert(index < groups);
		return proxy(*this, index);
	}

	//returns true if two bitgroup_sets are equal
	bool operator == (const bitgroup_vector & rval)
	{
		return vec == rval.vec;
	}

	//returns true if two bitgroup_sets are inequal
	bool operator != (const bitgroup_vector & rval)
	{
		return vec != rval.vec;
	}

	//bitwise AND one bitgroup_set with another
	bitgroup_vector & operator &= (const bitgroup_vector & rval)
	{
		size_t small = vec.size() < rval.vec.size() ? vec.size() : rval.vec.size();
		for(size_t x=0; x<small; ++x){
			vec[x] &= rval.vec[x];
		}
		return *this;
	}

	//bitwise XOR one bitgroup_set with another
	bitgroup_vector & operator ^= (const bitgroup_vector & rval)
	{
		size_t small = vec.size() < rval.vec.size() ? vec.size() : rval.vec.size();
		for(size_t x=0; x<small; ++x){
			vec[x] ^= rval.vec[x];
		}
		return *this;
	}

	//bitwise OR one bitgroup_set with another
	bitgroup_vector & operator |= (const bitgroup_vector & rval)
	{
		size_t small = vec.size() < rval.vec.size() ? vec.size() : rval.vec.size();
		for(size_t x=0; x<small; ++x){
			vec[x] |= rval.vec[x];
		}
		return *this;
	}

	//bitwise NOT bitgroup_set
	bitgroup_vector & operator ~ ()
	{
		for(size_t x=0; x<vec.size(); ++x){
			vec[x] = ~vec[x];
		}
		return *this;
	}

	//assign a bitgroup_set
	bitgroup_vector & operator = (const bitgroup_vector & rval)
	{
		vec = rval.vec;
		return *this;
	}

	/*
	Sets all bitgroups to zero. The way this function works is faster than
	looping through the individual bitgroups and setting them to zero.
	*/
	void reset()
	{
		for(size_t x=0; x<vec.size(); ++x){
			vec[x] = '\0';
		}
	}

	//returns the number of bitgroups in the bitgroup_set
	unsigned size()
	{
		return group_size;
	}

	/*
	Get a number from the bitgroup_set.
	Note: it is syntactically nicer to use operator [].
	*/
	unsigned get_num(const size_t index)
	{
		unsigned byte_offset = index * group_size / 8;
		unsigned bit_offset = index * group_size % 8;
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
				//bitgroup spans to bytes
				temp = vec[byte_offset+1];
				temp <<= 16 - bit_offset - group_size;
				temp >>= 16 - bit_offset - group_size;
				temp <<= -bit_offset + 8;
				result |= temp;
			}
		}
		return result;
	}

	/*
	Set a number in the bitgroup_set.
	Note: it is syntactically nicer to use operator [].
	*/
	void set_num(const size_t index, const unsigned num)
	{
		assert(num <= max_group_value);
		unsigned byte_offset = index * group_size / 8;
		unsigned bit_offset = index * group_size % 8;
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

private:
	const unsigned groups;     //how many bit groups
	const unsigned group_size; //how many bits in each group

	/*
	The bitgroups are stored here.
	Note: When the group_size > 1 bitgroups can partially span two bytes.
	*/
	std::vector<unsigned char> vec;

	/*
	Maximum value a bitgroup can hold.
		2^(group_size) - 1
	*/
	unsigned max_group_value;
};
#endif
