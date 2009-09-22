#include "slot.hpp"

slot::slot(
	const file_info & FI,
	database::pool::proxy DB
):
	Hash_Tree(FI, DB),
	File(FI),
	hash(Hash_Tree.hash),
	tree_size(Hash_Tree.tree_size),
	file_size(Hash_Tree.file_size)
{
	boost::shared_ptr<std::vector<database::table::host::host_info> >
		H = database::table::host::lookup(hash, DB);
	if(H){
		host.insert(H->begin(), H->end());
	}
}

bool slot::complete()
{
	bool temp = true;
	{//begin lock scope
	boost::mutex::scoped_lock lock(Hash_Tree_mutex);
	if(!Hash_Tree.complete()){
		temp = false;
	}
	}//end lock scope

/*
	{//begin lock scope
	boost::mutex::scoped_lock lock(File_mutex);
	if(!File.complete()){
		temp = false;
	}
	}//end lock scope
*/
	return temp;
}
