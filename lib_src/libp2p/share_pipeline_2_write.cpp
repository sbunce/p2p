#include "share_pipeline_2_write.hpp"

share_pipeline_2_write::share_pipeline_2_write(
	share_info & Share_Info_in
):
	Share_Info(Share_Info_in),
	Share_Pipeline_1_Hash(Share_Info)
{
	write_thread = boost::thread(boost::bind(&share_pipeline_2_write::main_loop, this));
}

share_pipeline_2_write::~share_pipeline_2_write()
{
	write_thread.interrupt();
	write_thread.join();
}

void share_pipeline_2_write::main_loop()
{
	//yield to other threads during prorgram start
	boost::this_thread::yield();

	while(true){
		boost::this_thread::interruption_point();
		boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
		database::pool::proxy DB;
		DB->query("BEGIN TRANSACTION");
		while(boost::shared_ptr<share_pipeline_job> SPJ = Share_Pipeline_1_Hash.job()){
			if(SPJ->add){
				database::table::share::file_info FI(SPJ->File.hash, SPJ->File.file_size,
					SPJ->File.path, database::table::share::COMPLETE);
				database::table::share::add_entry(FI, DB);
				database::table::hash::set_state(FI.hash, database::table::hash::COMPLETE, DB);
			}else{
				database::table::share::delete_entry(SPJ->File.path, DB);
			}
		}
		DB->query("END TRANSACTION");
	}
}
