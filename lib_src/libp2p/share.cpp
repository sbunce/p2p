#include "share.hpp"

share::share()
{

}

share::~share()
{
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

void share::start()
{
	Share_Pipeline_2_Write.start();
}

void share::stop()
{
	Share_Pipeline_2_Write.stop();
}
