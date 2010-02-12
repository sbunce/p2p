#include "database_table_peer.hpp"

//BEGIN info
database::table::peer::info::info(
	const std::string & ID_in,
	const std::string & IP_in,
	const std::string & port_in
):
	ID(ID_in),
	IP(IP_in),
	port(port_in)
{

}

database::table::peer::info::info(
	const info & I
):
	ID(I.ID),
	IP(I.IP),
	port(I.port)
{

}
//END info

void database::table::peer::add(const info & Info,
	database::pool::proxy DB)
{
	std::stringstream ss;
	ss << "INSERT OR IGNORE INTO peer VALUES('" << Info.ID << "', '"
		<< Info.IP << "', '" << Info.port << "')";
	DB->query(ss.str());
}

void database::table::peer::remove(const std::string ID,
	database::pool::proxy DB)
{
	std::stringstream ss;
	ss << "DELETE FROM peer WHERE ID = '" << ID << "'";
	DB->query(ss.str());
}

static int resume_call_back(int columns, char ** response,
	char ** column_name, std::deque<database::table::peer::info> & peers)
{
	assert(columns == 3);
	assert(std::strcmp(column_name[0], "ID") == 0);
	assert(std::strcmp(column_name[1], "IP") == 0);
	assert(std::strcmp(column_name[2], "port") == 0);
	peers.push_back(database::table::peer::info(response[0], response[1], response[2]));
	return 0;
}

std::deque<database::table::peer::info> database::table::peer::resume(
	database::pool::proxy DB)
{
	std::deque<database::table::peer::info> peers;
	DB->query("SELECT ID, IP, port FROM peer",
		boost::bind(&resume_call_back, _1, _2, _3, boost::ref(peers)));
	return peers;
}
