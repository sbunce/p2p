#include "db_table_peer.hpp"

//BEGIN info
db::table::peer::info::info(
	const std::string & ID_in,
	const std::string & IP_in,
	const std::string & port_in
):
	ID(ID_in),
	IP(IP_in),
	port(port_in)
{

}

db::table::peer::info::info(
	const info & I
):
	ID(I.ID),
	IP(I.IP),
	port(I.port)
{

}
//END info

void db::table::peer::add(const info & Info,
	db::pool::proxy DB)
{
	std::stringstream ss;
	ss << "INSERT OR IGNORE INTO peer VALUES('" << Info.ID << "', '"
		<< Info.IP << "', '" << Info.port << "')";
	DB->query(ss.str());
}

void db::table::peer::remove(const std::string ID,
	db::pool::proxy DB)
{
	std::stringstream ss;
	ss << "DELETE FROM peer WHERE ID = '" << ID << "'";
	DB->query(ss.str());
}

static int resume_call_back(int columns, char ** response,
	char ** column_name, std::list<db::table::peer::info> & peers)
{
	assert(columns == 3);
	assert(std::strcmp(column_name[0], "ID") == 0);
	assert(std::strcmp(column_name[1], "IP") == 0);
	assert(std::strcmp(column_name[2], "port") == 0);
	peers.push_back(db::table::peer::info(response[0], response[1], response[2]));
	return 0;
}

std::list<db::table::peer::info> db::table::peer::resume(
	db::pool::proxy DB)
{
	std::list<db::table::peer::info> peers;
	DB->query("SELECT ID, IP, port FROM peer",
		boost::bind(&resume_call_back, _1, _2, _3, boost::ref(peers)));
	return peers;
}
