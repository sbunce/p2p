#include "slot.hpp"

slot::slot(
	const file_info & FI,
	database::pool::proxy DB
):
	Hash_Tree(FI, DB),
	File(FI)
{
	boost::shared_ptr<std::vector<database::table::host::host_info> >
		H = database::table::host::lookup(Hash_Tree.hash, DB);
	if(H){
		host.insert(H->begin(), H->end());
	}
}

bool slot::complete()
{
	return Hash_Tree.complete() && File.complete();
}

bool slot::has_file(const std::string & IP, const std::string & port)
{
	boost::mutex::scoped_lock lock(host_mutex);
	return host.find(database::table::host::host_info(IP, port)) != host.end();
}

void slot::merge_host(std::set<database::table::host::host_info> & host_in)
{
	boost::mutex::scoped_lock lock(host_mutex);
	host_in.insert(host.begin(), host.end());
}
