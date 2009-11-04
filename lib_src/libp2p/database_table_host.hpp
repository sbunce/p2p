//THREADSAFE
#ifndef H_DATABASE_TABLE_HOST
#define H_DATABASE_TABLE_HOST

//custom
#include "database.hpp"

//include
#include <boost/shared_ptr.hpp>

//standard
#include <sstream>
#include <string>
#include <vector>

namespace database{
namespace table{
class host
{
public:
	class host_info
	{
	public:
		host_info();
		host_info(
			const host_info & H
		);
		host_info(
			const std::string & IP_in,	
			const std::string & port_in
		);

		std::string IP;
		std::string port;

		bool operator == (const host_info & rval) const;
		bool operator != (const host_info & rval) const;
		bool operator < (const host_info & rval) const;
	};

	/*
	add:
		Associate a host with a hash. This is done when we know that a host has a
		file with the specified hash.
	lookup:
		Returns all the IPs associated with the hash. Returns an empty shared
		pointer if no records found.
	*/
	static void add(const std::string hash, const host_info & HI,
		database::pool::proxy DB = database::pool::proxy());
	static boost::shared_ptr<std::vector<host_info> > lookup(
		const std::string & hash, database::pool::proxy DB = database::pool::proxy());

private:
	host(){}
};
}//end of namespace table
}//end of namespace database
#endif
