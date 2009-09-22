#include "database_table_host.hpp"

//BEGIN host_info
database::table::host::host_info::host_info()
{

}

database::table::host::host_info::host_info(
	const host_info & H
):
	host(H.host),
	port(H.port)
{

}

database::table::host::host_info::host_info(
	const std::string & host_in,	
	const std::string & port_in
):
	host(host_in),
	port(port_in)
{

}

bool database::table::host::host_info::operator == (const host_info & rval) const
{
	return host == rval.host && port == rval.port;
}

bool database::table::host::host_info::operator != (const host_info & rval) const
{
	return !(*this == rval);
}

bool database::table::host::host_info::operator < (const host_info & rval) const
{
	return (host + port) < (rval.host + rval.port);
}
//END host_info

void database::table::host::add(const std::string hash, const host_info & HI,
	database::pool::proxy DB)
{
	std::stringstream ss;
	ss << "INSERT OR IGNORE INTO host VALUES('" << hash << "', '" << HI.host
		<< "', '" << HI.port << "')";
	DB->query(ss.str());
}

static int lookup_call_back(boost::shared_ptr<std::vector<database::table::host::host_info> > & host,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0] && response[1]);
	if(!host){
		host = boost::shared_ptr<std::vector<database::table::host::host_info> >(
			new std::vector<database::table::host::host_info>());
	}
	host->push_back(database::table::host::host_info(response[0], response[1]));
	return 0;
}

boost::shared_ptr<std::vector<database::table::host::host_info> >
database::table::host::lookup( const std::string & hash, database::pool::proxy DB)
{
	std::stringstream ss;
	ss << "SELECT host, port FROM host WHERE hash = '" << hash << "'";
	boost::shared_ptr<std::vector<host_info> > host;
	DB->query(ss.str(), &lookup_call_back, host);
	return host;
}
