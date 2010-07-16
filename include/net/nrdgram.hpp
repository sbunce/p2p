#ifndef H_NET_NRDGRAM
#define H_NET_NRDGRAM

//custom
#include "ndgram.hpp"

//include
#include <slz/slz.hpp>

namespace net{
class nrdgram : public socket_base
{
public:


private:

	class dgram : public slz::message<0>
	{
	public:
		SLZ_CTOR_ASSIGN(dgram)

		//only one of these must be set, never both
		slz::boolean<0> rel_ord; //set and true if reliable/ordered, else not set
		slz::boolean<1> rel;     //set and true if relialbe, else not set

		//set if rel_ord or rel set
		slz::uint<2> seq_num;

		//opaque payload of message
		slz::string<3> payload;

	private:
		void add()
		{
			add_field(rel_ord);
			add_field(rel);
			add_field(seq_num);
			add_field(payload);
		}

		void copy(const dgram & rval)
		{
			rel_ord = rval.rel_ord;
			rel = rval.rel;
			seq_num = rval.seq_num;
			payload = rval.payload;
		}
	};
/*
	class stream : public slz::message<1>
	{
	public:
		stream()
		{
			add_field(seq_num);
		}

		//always set to sequence number
		slz::uint<0> seq_num;
	};
*/
};
}//end namespace net
#endif
