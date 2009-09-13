#include "share_scan.hpp"

share_scan::share_scan(
	shared_files & Shared_Files_in
):
	Share_Scan_2_Write(Shared_Files_in)
{

}

share_scan::~share_scan()
{
	path::remove_temporary_hash_tree_files();
}
