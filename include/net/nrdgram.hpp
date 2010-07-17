#ifndef H_NET_NRDGRAM
#define H_NET_NRDGRAM

//custom
#include "ndgram.hpp"

//include
//#include <slz/slz.hpp>

//standard
#include <map>

namespace net{
class nrdgram : public socket_base
{
public:

	//open for communication to specific endpoint
	virtual void open(const endpoint & E)
	{

	}

private:
/*
	class dgram : public slz::message<0>
	{
	public:
		SLZ_CTOR_ASSIGN(dgram)

		//either one, or none, of these must be set
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

	class stream : public slz::message<1>
	{
	public:
		SLZ_CTOR_ASSIGN(stream)

		//always set to sequence number
		slz::uint<0> seq_num;

	private:
		void add()
		{
			add_field(seq_num);
		}

		void copy(const stream & rval)
		{
			seq_num = rval.seq_num;
		}
	};
*/
};
}//end namespace net
#endif
