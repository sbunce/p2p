#include "slot.hpp"

slot::slot(
	const file_info & FI
):
	Hash_Tree(FI),
	File(FI)
{

}

void slot::check()
{
/*
	Hash_Tree.check();
	if(Hash_Tree.complete()){

	}
*/
}

bool slot::complete()
{
	return Hash_Tree.complete() && File.complete();
}

unsigned slot::download_speed()
{
	return 0;
}

boost::uint64_t slot::file_size()
{
	return File.file_size;
}

const std::string & slot::hash()
{
	return Hash_Tree.hash;
}

std::string slot::name()
{
	int pos;
	if((pos = File.path.rfind('/')) != std::string::npos){
		return File.path.substr(pos+1);
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
