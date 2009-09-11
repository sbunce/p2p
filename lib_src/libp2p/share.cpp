#include "share.hpp"

share::share():
	Share_Pipeline_2_Write(Share_Info)
{

}

share::~share()
{
	path::remove_temporary_hash_tree_files();
}

boost::uint64_t share::bytes()
{
	return Share_Info.bytes();
}

boost::uint64_t share::files()
{
	return Share_Info.files();
}
