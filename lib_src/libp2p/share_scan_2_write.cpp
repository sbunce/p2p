#include "share_scan_2_write.hpp"

share_scan_2_write::share_scan_2_write(
	shared_files & Shared_Files_in
):
	Shared_Files(Shared_Files_in),
	Share_Scan_1_Hash(Shared_Files)
{
	write_thread = boost::thread(boost::bind(&share_scan_2_write::main_loop, this));
}

share_scan_2_write::~share_scan_2_write()
{
	write_thread.interrupt();
	write_thread.join();
}

void share_scan_2_write::main_loop()
{
	//yield to other threads during prorgram start
	boost::this_thread::yield();

	while(true){
		boost::this_thread::interruption_point();
		boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
		database::pool::proxy DB;
		DB->query("BEGIN TRANSACTION");
		while(boost::shared_ptr<share_scan_job> SSJ = Share_Scan_1_Hash.job()){
			if(SSJ->add){
				database::table::share::file_info FI(SSJ->File.hash, SSJ->File.file_size,
					SSJ->File.path, database::table::share::COMPLETE);
				database::table::share::add_entry(FI, DB);
				database::table::hash::set_state(FI.hash, database::table::hash::COMPLETE, DB);
			}else{
				database::table::share::delete_entry(SSJ->File.path, DB);
			}
		}
		DB->query("END TRANSACTION");
	}
}
