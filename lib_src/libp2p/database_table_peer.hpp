#ifndef H_DATABASE_TABLE_PEER
#define H_DATABASE_TABLE_PEER

//custom
#include "database.hpp"

//include
#include <boost/shared_ptr.hpp>
#include <SHA1.hpp>

//standard
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace database{
namespace table{
class peer
{
public:
	class info
	{
	public:
		info(
			const std::string & ID_in,
			const std::string & IP_in,
			const std::string & port_in
		);
		info(const info & I);

		std::string ID;
		std::string IP;
		std::string port;
	};

	/*
	add_peer:
		Add peer to table.
	remove_peer:
		Remove peer from table.
	resume:
		Returns all peers in table.
	*/
	static void add(const info & Info,
		database::pool::proxy DB = database::pool::proxy());
	static void remove(const std::string ID,
		database::pool::proxy DB = database::pool::proxy());
	static std::deque<info> resume(
		database::pool::proxy DB = database::pool::proxy());

private:
	peer(){}
};
}//end of namespace table
}//end of namespace database
#endif
