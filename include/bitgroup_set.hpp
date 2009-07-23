#ifndef H_BITGROUP_SET
#define H_BITGROUP_SET

//include
#include <logger.hpp>

//standard
#include <cmath>
#include <vector>

class bitgroup_set
{
public:
	bitgroup_set(
		const unsigned groups_in,
		const unsigned group_size_in
	):
		groups(groups_in),
		group_size(group_size_in),
		set((groups * group_size) % 8 == 0 ?
			((groups * group_size) / 8) : ((groups * group_size) / 8 + 1),
			'\0'
		)
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
	The proxy emulates a unsigned int to be able to read/write the bitgroup_set
	as if it were a normal array. This is needed because [] can't be overloaded
	to return a few bits (memory is only byte addressible!). This approach
	has some inherent limitations. Specifically the proxy cannot be treated the
	same as a unsigned int in all contexts.
	*/
	class proxy
	{
	public:
		proxy(
			const int index,
			const unsigned group_size_in,
			std::vector<unsigned char> & set_in
		):
			group_size(group_size_in),
			set(set_in),
			byte_offset(index * group_size / 8),
			bit_offset(index * group_size % 8)
		{}

		proxy & operator = (const unsigned rval)
		{
			set_num(rval);
			return *this;
		}
		proxy & operator = (const proxy & P)
		{
			set_num(P.get_num());
			return *this;
		}
		bool operator == (const unsigned rval)
		{
			return get_num() == rval;
		}
		bool operator == (const proxy & P)
		{
			return get_num() == P.get_num();
		}
		bool operator != (const unsigned rval)
		{
			return get_num() != rval;
		}
		bool operator != (const proxy & P)
		{
			return get_num() != P.get_num();
		}
		friend std::ostream & operator << (std::ostream & lval, const proxy & Proxy)
		{
			return lval << Proxy.get_num();
		}

	private:
		//same variables that exist in bitgroup_set
		const unsigned group_size;
		std::vector<unsigned char> & set;

		unsigned byte_offset; //offset from start of set
		unsigned bit_offset;  //offset from start of byte

		unsigned get_num() const
		{
			unsigned char temp, result = 0;
			if(bit_offset + group_size < 8){
				//bits on left don't belong to this bitgroup
				temp = set[byte_offset];
				temp <<= 8 - bit_offset - group_size;
				temp >>= 8 - group_size;
				result |= temp;
			}else{
				//bits on left all belong to this bitgroup
				temp = set[byte_offset];
				temp >>= bit_offset;
				result |= temp;
				if(bit_offset + group_size != 8){
					//bitgroup spans to bytes
					temp = set[byte_offset+1];
					temp <<= 16 - bit_offset - group_size;
					temp >>= 16 - bit_offset - group_size;
					temp <<= -bit_offset + 8;
					result |= temp;
				}
			}
			return (unsigned)result;
		}

		void set_num(const unsigned num)
		{
			assert(num < std::pow(2, group_size));
			unsigned char temp;
			set[byte_offset] <<= 8 - bit_offset;
			set[byte_offset] >>= 8 - bit_offset;
			temp = num;
			temp <<= bit_offset;
			set[byte_offset] |= temp;
			if(bit_offset + group_size > 8){
				//bitgroup spans two bytes
				set[byte_offset+1] >>= 8 - bit_offset + group_size;
				set[byte_offset+1] <<= 8 - bit_offset + group_size;
				temp = num;
				temp >>= -bit_offset + 8;
				set[byte_offset+1] |= temp;
			}
		}
	};

	//returns proxy object (see documentation for proxy)
	proxy operator [](const int index)
	{
		assert(index < groups);
		return proxy(index, group_size, set);
	}

	//returns true if two bitgroup_sets are equal
	bool operator == (const bitgroup_set & rval)
	{
		return set == rval.set;
	}

	//returns true if two bitgroup_sets are inequal
	bool operator != (const bitgroup_set & rval)
	{
		return set != rval.set;
	}

	//bitwise AND one bitgroup_set with another
	bitgroup_set & operator &= (const bitgroup_set & rval)
	{
		int small = set.size() < rval.set.size() ? set.size() : rval.set.size();
		for(int x=0; x<small; ++x){
			set[x] &= rval.set[x];
		}
		return *this;
	}

	//bitwise XOR one bitgroup_set with another
	bitgroup_set & operator ^= (const bitgroup_set & rval)
	{
		int small = set.size() < rval.set.size() ? set.size() : rval.set.size();
		for(int x=0; x<small; ++x){
			set[x] ^= rval.set[x];
		}
		return *this;
	}

	//bitwise OR one bitgroup_set with another
	bitgroup_set & operator |= (const bitgroup_set & rval)
	{
		int small = set.size() < rval.set.size() ? set.size() : rval.set.size();
		for(int x=0; x<small; ++x){
			set[x] |= rval.set[x];
		}
		return *this;
	}

	//bitwise NOT bitgroup_set
	bitgroup_set & operator ~ ()
	{
		for(int x=0; x<set.size(); ++x){
			set[x] = ~set[x];
		}
		return *this;
	}

	//assign a bitgroup_set
	bitgroup_set & operator = (const bitgroup_set & rval)
	{
		set = rval.set;
		return *this;
	}

	/*
	Sets all bitgroups to zero. The way this function works is faster than
	looping through the individual bitgroups and setting them to zero.
	*/
	void reset()
	{
		for(int x=0; x<set.size(); ++x){
			set[x] = '\0';
		}
	}

	//returns the number of bitgroups in the bitgroup_set
	unsigned size()
	{
		return group_size;
	}

private:
	const unsigned groups;     //how many bit groups
	const unsigned group_size; //how many bits in each group
	std::vector<unsigned char> set;
};
#endif
