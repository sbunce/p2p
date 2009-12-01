#include "database_table_join.hpp"

static int resume_hash_call_back(int columns, char ** response,
	char ** column_name, std::set<std::string> & hash)
{
	assert(columns == 1);
	assert(std::strcmp(column_name[0], "hash") == 0);
	hash.insert(response[0]);
	return 0;
}

std::set<std::string> database::table::join::resume_hash(const std::string & peer_ID,
	database::pool::proxy DB)
{
	std::stringstream ss;
	ss << "SELECT DISTINCT share.hash FROM share, source WHERE share.state = 0 "
		"AND share.hash = source.hash AND source.ID = '" << peer_ID << "'";
	std::set<std::string> hash;
	DB->query(ss.str(), boost::bind(&resume_hash_call_back, _1, _2, _3,
		boost::ref(hash)));
	return hash;
}

static int resume_peers_call_back(int columns, char ** response,
	char ** column_name, std::set<std::pair<std::string, std::string> > & peers)
{
	assert(columns == 2);
	assert(std::strcmp(column_name[0], "IP") == 0);
	assert(std::strcmp(column_name[1], "port") == 0);
	peers.insert(std::make_pair(response[0], response[1]));
	return 0;
}

std::set<std::pair<std::string, std::string> > database::table::join::resume_peers(
	database::pool::proxy DB)
{
	std::set<std::pair<std::string, std::string> > peers;
	DB->query("SELECT DISTINCT peer.IP, peer.port FROM peer, share, source "
		"WHERE share.state = 0 AND share.hash = source.hash AND source.ID = peer.ID",
		boost::bind(&resume_peers_call_back, _1, _2, _3, boost::ref(peers)));
	return peers;
}
