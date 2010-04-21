#ifndef H_DB_TABLE_PEER
#define H_DB_TABLE_PEER

//custom
#include "db_all.hpp"

//include
#include <boost/shared_ptr.hpp>
#include <SHA1.hpp>

//standard
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace db{
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
		db::pool::proxy DB = db::pool::proxy());
	static void remove(const std::string ID,
		db::pool::proxy DB = db::pool::proxy());
	static std::vector<info> resume(
		db::pool::proxy DB = db::pool::proxy());

private:
	peer(){}
};
}//end of namespace table
}//end of namespace database
#endif
