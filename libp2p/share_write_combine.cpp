#include "share_write_combine.hpp"

share_write_combine::share_write_combine()
{
	write_combine_thread = boost::thread(boost::bind(&share_index::write_combine, this));
}

void share_write_combine::main_loop()
{
	database::connection DB(path::database());
	std::vector<share_scan::job_info> temp;
	while(true){
		if(boost::this_thread::interruption_requested()){
			/*
			Make sure all jobs written to database when thread terminated.
			*/
			boost::mutex::scoped_lock lock(write_combine_job_mutex);
			if(write_combine_job.empty()){
				break;
			}
		}else{
			//write database at this interval + previous time to write database
			portable_sleep::ms(1000);
		}

		/*
		Copy the write_combine_job vector so we don't have to hold the lock to it
		while we iterate through the jobs and do queries.
		*/
		{//begin lock scope
		boost::mutex::scoped_lock lock(write_combine_job_mutex);
		temp.assign(write_combine_job.begin(), write_combine_job.end());
		write_combine_job.clear();
		}//end lock scope

		DB.query("BEGIN TRANSACTION");
		std::vector<share_scan::job_info>::iterator iter_cur = temp.begin(),
			iter_end = temp.end();
		while(iter_cur != iter_end){
			if(iter_cur->add){
				//job to add file to share table
				database::table::share::add_entry(iter_cur->hash, iter_cur->file_size, iter_cur->path, DB);
				database::table::hash::set_state(iter_cur->hash,
					hash_tree::tree_info::file_size_to_tree_size(iter_cur->file_size),
					database::table::hash::COMPLETE, DB);
			}else{
				//job to remove file from share table
				database::table::share::delete_entry(iter_cur->path, DB);
			}
			++iter_cur;
		}
		DB.query("END TRANSACTION");
	}
}
