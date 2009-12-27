#include "slot.hpp"

slot::slot(
	const file_info & FI
):
	Hash_Tree(FI)
{

}

void slot::check()
{

}

bool slot::complete()
{
	//return Hash_Tree.tree_complete() && Hash_Tree.file_complete();
	return false;
}

unsigned slot::download_speed()
{
	return 0;
}

boost::uint64_t slot::file_size()
{
	return Hash_Tree.file_size;
}

const std::string & slot::hash()
{
	return Hash_Tree.hash;
}

std::string slot::name()
{
	int pos;
	if((pos = Hash_Tree.path.rfind('/')) != std::string::npos){
		return Hash_Tree.path.substr(pos+1);
	}else{
		return "";
	}
}

unsigned slot::percent_complete()
{
	return 69;
}

unsigned slot::upload_speed()
{
	return 0;
}
