#include "share_scan_0_scan.hpp"

share_scan_0_scan::share_scan_0_scan(
	share & Share_in
):
	resumed(false),
	Share(Share_in)
{
	scan_thread = boost::thread(boost::bind(&share_scan_0_scan::main_loop, this));
}

share_scan_0_scan::~share_scan_0_scan()
{
	scan_thread.interrupt();
	scan_thread.join();
}

void share_scan_0_scan::block_on_max_jobs()
{
	boost::mutex::scoped_lock lock(job_queue_mutex);
	while(job_queue.size() >= settings::SHARE_SCAN_RATE){
		job_queue_max_cond.wait(job_queue_mutex);
	}
}

void share_scan_0_scan::block_until_resumed()
{
	boost::mutex::scoped_lock lock(resumed_mutex);
	while(!resumed){
		resumed_cond.wait(resumed_mutex);
	}
}

boost::shared_ptr<share_scan_job> share_scan_0_scan::job()
{
	boost::mutex::scoped_lock lock(job_queue_mutex);
	while(job_queue.empty()){
		job_queue_cond.wait(job_queue_mutex);
	}
	boost::shared_ptr<share_scan_job> SSJ = job_queue.front();
	job_queue.pop_front();
	job_queue_max_cond.notify_one();
	return SSJ;
}

int share_scan_0_scan::resume_call_back(database::pool::proxy & DB,
	int columns_retrieved, char ** response, char ** column_name)
{
	if(boost::this_thread::interruption_requested()){
		return 1;
	}
	assert(response[0] && response[1] && response[2] && response[3]);
	namespace fs = boost::filesystem;
	file_info FI;
	FI.hash = response[0];
	FI.path = response[1];
	std::stringstream ss;
	ss << response[2];
	ss >> FI.file_size;
	ss.str(""); ss.clear();
	ss << response[3];
	int temp;
	ss >> temp;
	database::table::share::state share_state
		= reinterpret_cast<database::table::share::state &>(temp);
	Share.insert_update(FI);
	if(share_state != database::table::share::complete){
		Share.get_slot(FI.hash, DB);
	}
	return 0;
}

void share_scan_0_scan::main_loop()
{
	//yield to other threads during prorgram start
	boost::this_thread::sleep(boost::posix_time::milliseconds(100));

	//read in existing share information from database
	std::stringstream ss;
	ss << "SELECT hash, path, file_size, state FROM share";

	{//scope used to destroy DB
	database::pool::proxy DB;
	DB->query(ss.str(), this, &share_scan_0_scan::resume_call_back, DB);
	}

	{//begin lock scope
	boost::mutex::scoped_lock lock(resumed_mutex);
	resumed = true;
	resumed_cond.notify_all();
	}//end lock scope

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
						share::const_file_iterator share_iter = Share.lookup_path(path);
						if((share_iter == Share.end_file() || share_iter->file_size != file_size)
							&& !Share.is_downloading(path))
						{
							//file not downloading and it is new or changed size
							boost::shared_ptr<share_scan_job> SSJ(new share_scan_job(true,
								file_info("", path, file_size)));
							Share.insert_update(SSJ->FI);
							{//begin lock scope
							boost::mutex::scoped_lock lock(job_queue_mutex);
							job_queue.push_back(SSJ);
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
		for(share::const_file_iterator iter_cur = Share.begin_file(),
			iter_end = Share.end_file(); iter_cur != iter_end; ++iter_cur)
		{
			boost::this_thread::interruption_point();
			boost::this_thread::sleep(scan_delay);
			if(!boost::filesystem::exists(iter_cur->path)){
				Share.erase(iter_cur->path);
				boost::shared_ptr<share_scan_job> SSJ(
					new share_scan_job(false, *iter_cur));
				{//begin lock scope
				boost::mutex::scoped_lock lock(job_queue_mutex);
				job_queue.push_back(SSJ);
				job_queue_cond.notify_one();
				}//end lock scope
			}
		}
	}
}
