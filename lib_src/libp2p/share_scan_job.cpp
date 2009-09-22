#include "share_scan_job.hpp"

share_scan_job::share_scan_job()
{

}

share_scan_job::share_scan_job(
	const bool add_in,
	const file_info & FI_in
):
	add(add_in),
	FI(FI_in)
{

}
