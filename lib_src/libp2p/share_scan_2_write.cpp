#include "share_scan_2_write.hpp"

share_scan_2_write::share_scan_2_write()
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
	bool skip_sleep = false;
	while(true){
		boost::this_thread::interruption_point();
		if(skip_sleep){
			skip_sleep = false;
		}else{
			boost::this_thread::sleep(boost::posix_time::milliseconds(100));
		}
		database::pool::proxy DB;
		DB->query("BEGIN TRANSACTION");
		int count = 0;
		while(boost::shared_ptr<file_info> FI = Share_Scan_1_Hash.job()){
			if(FI->file_size == 0){
				database::table::share::delete_entry(FI->path, DB);
			}else{
				database::table::share::add_entry(
					database::table::share::file_info(*FI, database::table::share::complete), DB);
				database::table::hash::set_state(FI->hash, database::table::hash::complete, DB);
			}
			if(++count > settings::SHARE_BUFFER_SIZE){
				skip_sleep = true;
				break;
			}
		}
		DB->query("END TRANSACTION");
	}
}
