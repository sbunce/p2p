#ifndef H_BITGROUP_SET
#define H_BITGROUP_SET

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
		max_group_value(std::pow(2, group_size))
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
	The proxy object is returned from functions instead of an integer because
	special stuff needs to be done to read/write to the bitgroup_vector.
	*/
	class proxy
	{
		friend class bitgroup_vector;
	public:
		operator unsigned()
		{
			return BGV->get_num(index);
		}
		proxy & operator = (const unsigned num)
		{
			BGV->set_num(index, num);
			return *this;
		}
		friend std::ostream & operator << (std::ostream & lval, const proxy & Proxy)
		{
			return lval << Proxy.BGV->get_num(Proxy.index);
		}

	private:
		proxy(
			bitgroup_vector * BGV_in,
			const size_t index_in
		):
			BGV(BGV_in),
			index(index_in)
		{}

		bitgroup_vector * BGV;
		size_t index;
	};

	class iterator : public std::iterator<std::random_access_iterator_tag, unsigned>
	{
		friend class bitgroup_vector;
	public:
		iterator(){}
		iterator & operator = (const iterator & rval)
		{
			index = rval.index;
			BGV = rval.BGV;
			return *this;
		}
		bool operator == (const iterator & rval) const
		{
			return index == rval.index;
		}
		bool operator != (const iterator & rval) const
		{
			return index != rval.index;
		}
		proxy operator * ()
		{
			return proxy(BGV, index);
		}
		//operator -> doesn't have to be defined because ->m is not well-defined
		iterator & operator ++ ()
		{
			++index;
			return *this;
		}
		iterator operator ++ (int)
		{
			size_t index_tmp = index;
			++index;
			return iterator(BGV, index_tmp);
		}
		iterator & operator -- ()
		{
			--index;
			return *this;
		}
		iterator operator -- (int)
		{
			size_t index_tmp = index;
			--index;
			return iterator(BGV, index_tmp);
		}
		iterator operator + (const unsigned rval)
		{
			return iterator(BGV, index + rval);
		}
		iterator operator + (const iterator & rval)
		{
			return iterator(BGV, index + rval.index);
		}
		iterator operator - (const unsigned rval)
		{
			return iterator(BGV, index - rval);
		}
		ptrdiff_t operator - (const iterator & rval)
		{
			return (index < rval.index) ? rval.index - index : index - rval.index;
		}
		iterator & operator += (const unsigned rval)
		{
			index += rval;
			return *this;
		}
		iterator & operator += (const iterator & rval)
		{
			index += rval.index;
			return *this;
		}
		iterator & operator -= (const unsigned rval)
		{
			index -= rval;
			return *this;
		}
		iterator & operator -= (const iterator & rval)
		{
			index -= rval.index;
			return *this;
		}
		proxy operator [] (const size_t offset)
		{
			return proxy(BGV, index + offset);
		}
		bool operator < (const iterator & rval) const
		{
			return index < rval.index;
		}
		bool operator > (const iterator & rval) const
		{
			return index > rval.index;
		}
		bool operator >= (const iterator & rval) const
		{
			return index >= rval.index;
		}
		bool operator <= (const iterator & rval) const
		{
			return index <= rval.index;
		}

	private:
		iterator(
			bitgroup_vector * BGV_in,
			const size_t index_in
		):
			BGV(BGV_in),
			index(index_in)
		{}

		bitgroup_vector * BGV; //bitgroup_vector from which the iterator was created
		size_t index;          //current index
	};

	//returns proxy object (see documentation for proxy)
	proxy operator [](const size_t index)
	{
		assert(index < groups);
		return proxy(this, index);
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

	iterator begin()
	{
		return iterator(this, 0);
	}

	iterator end()
	{
		return iterator(this, groups);
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
	Note: it is syntactically nicer to use the operators.
	*/
	unsigned get_num(const unsigned index)
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
		return (unsigned)result;
	}

	/*
	Set a number in the bitgroup_set.
	Note: it is syntactically nicer to use the operators.
	*/
	void set_num(const unsigned index, const unsigned num)
	{
		assert(num < max_group_value);
		unsigned byte_offset = index * group_size / 8;
		unsigned bit_offset = index * group_size % 8;
		unsigned char temp;
		vec[byte_offset] <<= 8 - bit_offset;
		vec[byte_offset] >>= 8 - bit_offset;
		temp = num;
		temp <<= bit_offset;
		vec[byte_offset] |= temp;
		if(bit_offset + group_size > 8){
			//bitgroup spans two bytes
			vec[byte_offset+1] >>= 8 - bit_offset + group_size;
			vec[byte_offset+1] <<= 8 - bit_offset + group_size;
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

	//maximum value a group can hold (used for asserts)
	const unsigned max_group_value;
};
#endif
