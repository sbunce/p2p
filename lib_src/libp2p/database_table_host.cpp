#include "database_table_host.hpp"

//BEGIN host_info
database::table::host::host_info::host_info()
{

}

database::table::host::host_info::host_info(
	const host_info & H
):
	IP(H.IP),
	port(H.port)
{

}

database::table::host::host_info::host_info(
	const std::string & IP_in,	
	const std::string & port_in
):
	IP(IP_in),
	port(port_in)
{

}

bool database::table::host::host_info::operator == (const host_info & rval) const
{
	return IP == rval.IP && port == rval.port;
}

bool database::table::host::host_info::operator != (const host_info & rval) const
{
	return !(*this == rval);
}

bool database::table::host::host_info::operator < (const host_info & rval) const
{
	return (IP + port) < (rval.IP + rval.port);
}
//END host_info

void database::table::host::add(const std::string hash, const host_info & HI,
	database::pool::proxy DB)
{
	std::stringstream ss;
	ss << "INSERT OR IGNORE INTO host VALUES('" << hash << "', '" << HI.IP
		<< "', '" << HI.port << "')";
	DB->query(ss.str());
}

static int lookup_call_back(
	boost::reference_wrapper<boost::shared_ptr<std::vector<database::table::host::host_info> > > host,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(columns_retrieved == 2);
	if(!host.get()){
		/*
		This function will be called multiple times or no times. If the vector
		hasn't already been created then create it.
		*/
		host.get() = boost::shared_ptr<std::vector<database::table::host::host_info> >(
			new std::vector<database::table::host::host_info>());
	}
	host.get()->push_back(database::table::host::host_info(response[0], response[1]));
	return 0;
}

boost::shared_ptr<std::vector<database::table::host::host_info> >
database::table::host::lookup( const std::string & hash, database::pool::proxy DB)
{
	std::stringstream ss;
	ss << "SELECT IP, port FROM host WHERE hash = '" << hash << "'";
	boost::shared_ptr<std::vector<host_info> > host;
	DB->query(ss.str(), &lookup_call_back, boost::ref(host));
	return host;
}
