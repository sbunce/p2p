#include "share_pipeline_0_scan.hpp"

share_pipeline_0_scan::share_pipeline_0_scan(
	share_info & Share_Info_in
):
	Share_Info(Share_Info_in)
{
	scan_thread = boost::thread(boost::bind(&share_pipeline_0_scan::main_loop, this));
}

share_pipeline_0_scan::~share_pipeline_0_scan()
{
	scan_thread.interrupt();
	scan_thread.join();
}

void share_pipeline_0_scan::block_on_max_jobs()
{
	boost::mutex::scoped_lock lock(job_queue_mutex);
	while(job_queue.size() >= settings::SHARE_SCAN_RATE){
		job_queue_max_cond.wait(job_queue_mutex);
	}
}

boost::shared_ptr<share_pipeline_job> share_pipeline_0_scan::job()
{
	boost::mutex::scoped_lock lock(job_queue_mutex);
	while(job_queue.empty()){
		job_queue_cond.wait(job_queue_mutex);
	}
	boost::shared_ptr<share_pipeline_job> SPJ = job_queue.front();
	job_queue.pop_front();
	job_queue_max_cond.notify_one();
	return SPJ;
}

int resume_call_back(share_info & Share_Info, int columns_retrieved, char ** response,
	char ** column_name)
{
	assert(response[0] && response[1] && response[2]);
	namespace fs = boost::filesystem;

	//path
	fs::path path = fs::system_complete(fs::path(response[1], fs::native));

	//file size
	boost::uint64_t file_size;
	std::stringstream ss;
	ss << response[2];
	ss >> file_size;

	Share_Info.insert_update(share_info::file(response[0], path.string(), file_size));

	if(boost::this_thread::interruption_requested()){
		return 1;
	}else{
		return 0;
	}
}

void share_pipeline_0_scan::main_loop()
{
	//yield to other threads during prorgram start
	boost::this_thread::yield();

	//read in existing share information from database
	database::pool::get_proxy()->query(
		"SELECT hash, path, file_size FROM share WHERE state = 1",
		&resume_call_back, Share_Info);

	boost::filesystem::path share_path(boost::filesystem::system_complete(
		boost::filesystem::path(path::share(), boost::filesystem::native)));
	boost::posix_time::milliseconds scan_delay(1000 / settings::SHARE_SCAN_RATE);

	while(true){
		boost::this_thread::interruption_point();
		boost::this_thread::sleep(scan_delay);
		try{
			//scan share
			boost::filesystem::recursive_directory_iterator iter_cur(share_path), iter_end;
			while(iter_cur != iter_end){
				boost::this_thread::interruption_point();
				boost::this_thread::sleep(scan_delay);
				block_on_max_jobs();
				if(boost::filesystem::is_symlink(iter_cur->path().parent_path())){
					//traversed to symlink directory, go back up and skip
					iter_cur.pop();
				}else{
					if(boost::filesystem::is_regular_file(iter_cur->status())){
						std::string path = iter_cur->path().string();
						boost::uint64_t file_size = boost::filesystem::file_size(path);
						boost::shared_ptr<const share_info::file> F = Share_Info.lookup_path(path);
						if(!F || F->file_size != file_size){
							//file hasn't been seen before, or it changed size
							boost::shared_ptr<share_pipeline_job> SPJ(
								new share_pipeline_job(true, share_info::file("", path, file_size)));
							Share_Info.insert_update(SPJ->File);

							{//begin lock scope
							boost::mutex::scoped_lock lock(job_queue_mutex);
							job_queue.push_back(SPJ);
							job_queue_cond.notify_one();
							}//end lock scope
						}
					}
					++iter_cur;
				}
			}
		}catch(std::exception & ex){
			LOGGER << ex.what();
		}

		//remove missing
		for(share_info::const_iterator iter_cur = Share_Info.begin(),
			iter_end = Share_Info.end(); iter_cur != iter_end; ++iter_cur)
		{
			boost::this_thread::interruption_point();
			boost::this_thread::sleep(scan_delay);
			if(!boost::filesystem::exists(iter_cur->path)){
				//erasing from Share_Info doesn't invalidate iterator
				Share_Info.erase(iter_cur);
				boost::shared_ptr<share_pipeline_job> SPJ(
					new share_pipeline_job(false, *iter_cur));
				{//begin lock scope
				boost::mutex::scoped_lock lock(job_queue_mutex);
				job_queue.push_back(SPJ);
				job_queue_cond.notify_one();
				}//end lock scope
			}
		}
	}
}
