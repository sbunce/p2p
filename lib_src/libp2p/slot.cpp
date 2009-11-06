#include "slot.hpp"

slot::slot(
	const file_info & FI,
	database::pool::proxy DB
):
	Hash_Tree(FI, DB),
	File(FI)
{
	host = database::table::host::lookup(FI.hash, DB);
}

bool slot::complete()
{
	return Hash_Tree.complete() && File.complete();
}

bool slot::has_file(const std::string & IP, const std::string & port)
{
	boost::mutex::scoped_lock lock(host_mutex);
	return host.find(std::make_pair(IP, port)) != host.end();
}

void slot::merge_host(std::set<std::pair<std::string, std::string> > & host_in)
{
	boost::mutex::scoped_lock lock(host_mutex);
	host_in.insert(host.begin(), host.end());
}
