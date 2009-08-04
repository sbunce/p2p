#include "share_pipeline_2_write.hpp"

share_pipeline_2_write::share_pipeline_2_write()
{

}

void share_pipeline_2_write::main_loop()
{
	/*
	Thread takes a lot of CPU time to bring up. This can increase the time it
	takes to instantiate libp2p. This delay speeds up library instantiation. This
	is important for fast GUI startup time.
	*/
	portable_sleep::yield();

	std::vector<share_pipeline_job> temp;
	share_pipeline_job info;
	while(true){
		boost::this_thread::interruption_point();

		//interval between writes
		portable_sleep::ms(1000);

		//use temp buffer to empty the hash stage buffer ASAP
		temp.clear();
		while(Share_Pipeline_1_Hash.get_job(info)){
			temp.push_back(info);
		}

		if(!temp.empty()){
			database::pool::proxy DB;
			DB->query("BEGIN TRANSACTION");
			for(std::vector<share_pipeline_job>::iterator iter_cur = temp.begin(),
				iter_end = temp.end(); iter_cur != iter_end; ++iter_cur)
			{
				if(iter_cur->Type == share_pipeline_job::HASH_FILE){
					//job to add file to share table
					database::table::share::add_entry(iter_cur->hash,
						iter_cur->file_size, iter_cur->path, DB);
					database::table::hash::set_state(iter_cur->hash,
						hash_tree::file_size_to_tree_size(iter_cur->file_size),
						database::table::hash::COMPLETE, DB);
				}else if(iter_cur->Type == share_pipeline_job::REMOVE_FILE){
					//job to remove file from share table
					database::table::share::delete_entry(iter_cur->path, DB);
				}else{
					LOGGER << "programmer error";
					exit(1);
				}
			}
			DB->query("END TRANSACTION");
		}
	}
}

boost::uint64_t share_pipeline_2_write::size_bytes()
{
	return Share_Pipeline_1_Hash.size_bytes();
}

boost::uint64_t share_pipeline_2_write::size_files()
{
	return Share_Pipeline_1_Hash.size_files();
}

void share_pipeline_2_write::start()
{
	write_thread = boost::thread(boost::bind(&share_pipeline_2_write::main_loop, this));
	Share_Pipeline_1_Hash.start();
}

void share_pipeline_2_write::stop()
{
	Share_Pipeline_1_Hash.stop();
	write_thread.interrupt();
	write_thread.join();
}
