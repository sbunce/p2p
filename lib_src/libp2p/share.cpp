#include "share.hpp"

share::share()
{
	database::init::hash();
	database::init::share();
	Share_Pipeline_2_Write.start();
}

share::~share()
{
	Share_Pipeline_2_Write.stop();
	path::remove_temporary_hash_tree_files();
}

boost::uint64_t share::size_bytes()
{
	return Share_Pipeline_2_Write.size_bytes();
}

boost::uint64_t share::size_files()
{
	return Share_Pipeline_2_Write.size_files();
}
