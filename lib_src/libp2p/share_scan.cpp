#include "share_scan.hpp"

share_scan::share_scan()
{

}

share_scan::~share_scan()
{
	path::remove_temporary_hash_tree_files();
}

void share_scan::block_until_resumed()
{
	Share_Scan_2_Write.block_until_resumed();
}
