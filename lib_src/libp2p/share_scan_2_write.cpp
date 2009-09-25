#include "share_scan_2_write.hpp"

share_scan_2_write::share_scan_2_write(
	share & Share_in
):
	Share(Share_in),
	Share_Scan_1_Hash(Share)
{
	write_thread = boost::thread(boost::bind(&share_scan_2_write::main_loop, this));
}

share_scan_2_write::~share_scan_2_write()
{
	write_thread.interrupt();
	write_thread.join();
}

void share_scan_2_write::block_until_resumed()
{
	Share_Scan_1_Hash.block_until_resumed();
}

void share_scan_2_write::main_loop()
{
	while(true){
		boost::this_thread::interruption_point();
		boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
		database::pool::proxy DB;
		DB->query("BEGIN TRANSACTION");
		while(boost::shared_ptr<share_scan_job> SSJ = Share_Scan_1_Hash.job()){
			if(SSJ->add){
				database::table::share::file_info FI(SSJ->FI.hash, SSJ->FI.file_size,
					SSJ->FI.path, database::table::share::complete);
				database::table::share::add_entry(FI, DB);
				database::table::hash::set_state(FI.hash, database::table::hash::complete, DB);
			}else{
				database::table::share::delete_entry(SSJ->FI.path, DB);
			}
		}
		DB->query("END TRANSACTION");
	}
}
