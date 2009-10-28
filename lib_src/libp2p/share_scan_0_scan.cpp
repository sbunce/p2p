#include "share_scan_0_scan.hpp"

share_scan_0_scan::share_scan_0_scan():
	resumed(false)
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
	while(job_queue.size() >= settings::SHARE_BUFFER_SIZE){
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

boost::shared_ptr<file_info> share_scan_0_scan::job()
{
	boost::mutex::scoped_lock lock(job_queue_mutex);
	while(job_queue.empty()){
		job_queue_cond.wait(job_queue_mutex);
	}
	boost::shared_ptr<file_info> FI = job_queue.front();
	job_queue.pop_front();
	job_queue_max_cond.notify_one();
	return FI;
}

int share_scan_0_scan::resume_call_back(
	boost::reference_wrapper<database::pool::proxy> DB,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(columns_retrieved == 5);
	if(boost::this_thread::interruption_requested()){
		return 1;
	}
	try{
		file_info FI;
		FI.hash = response[0];
		FI.path = response[1];
		FI.file_size = boost::lexical_cast<boost::uint64_t>(response[2]);
		FI.last_write_time = boost::lexical_cast<std::time_t>(response[3]);
		int temp = boost::lexical_cast<int>(response[4]);
		database::table::share::state state = reinterpret_cast<database::table::share::state &>(temp);
		share::singleton().insert_update(FI);
		if(state != database::table::share::complete){
			share::singleton().get_slot(FI.hash, DB.get());
		}
	}catch(const std::exception & e){
		LOGGER << e.what();
	}
	return 0;
}

void share_scan_0_scan::main_loop()
{
	//read in existing share information from database
	std::stringstream ss;
	ss << "SELECT hash, path, file_size, last_write_time, state FROM share";

	{//scope used to destroy DB
	database::pool::proxy DB;
	DB->query(ss.str(), this, &share_scan_0_scan::resume_call_back, boost::ref(DB));
	}

	{//begin lock scope
	boost::mutex::scoped_lock lock(resumed_mutex);
	resumed = true;
	resumed_cond.notify_all();
	}//end lock scope

	boost::filesystem::path share_path(boost::filesystem::system_complete(
		boost::filesystem::path(path::share(), boost::filesystem::native)));
	boost::posix_time::milliseconds scan_delay(20);

	/*
	If a new file is found we don't sleep before we iterate to the next file.
	This takes advantage of the fact that new files are often added in groups.
	*/
	bool skip_sleep = false;

	while(true){
		boost::this_thread::interruption_point();
		boost::this_thread::sleep(scan_delay);
		try{
			//scan share
			boost::filesystem::recursive_directory_iterator iter_cur(share_path), iter_end;
			while(iter_cur != iter_end){
				boost::this_thread::interruption_point();
				if(skip_sleep){
					skip_sleep = false;
				}else{
					boost::this_thread::sleep(scan_delay);
				}
				block_on_max_jobs();
				if(boost::filesystem::is_symlink(iter_cur->path().parent_path())){
					//traversed to symlink directory, go back up and skip
					iter_cur.pop();
				}else{
					if(boost::filesystem::is_regular_file(iter_cur->status())){
						std::string path = iter_cur->path().string();
						boost::uint64_t file_size = boost::filesystem::file_size(path);
						std::time_t last_write_time = boost::filesystem::last_write_time(iter_cur->path());
						share::const_file_iterator share_iter = share::singleton().lookup_path(path);
						if(share_iter == share::singleton().end_file()
							|| (!share::singleton().is_downloading(path) && (share_iter->file_size != file_size
							|| share_iter->last_write_time != last_write_time)))
						{
							//new file, or modified file
							boost::shared_ptr<file_info> FI(new file_info("", path,
								file_size, last_write_time));
							share::singleton().insert_update(*FI);
							{//begin lock scope
							boost::mutex::scoped_lock lock(job_queue_mutex);
							job_queue.push_back(FI);
							job_queue_cond.notify_one();
							}//end lock scope
							skip_sleep = true;
						}
					}
					++iter_cur;
				}
			}
		}catch(std::exception & ex){
			LOGGER << ex.what();
		}

		//remove missing
		for(share::const_file_iterator iter_cur = share::singleton().begin_file(),
			iter_end = share::singleton().end_file(); iter_cur != iter_end; ++iter_cur)
		{
			boost::this_thread::interruption_point();
			if(skip_sleep){
				skip_sleep = false;
			}else{
				boost::this_thread::sleep(scan_delay);
			}
			block_on_max_jobs();
			if(!boost::filesystem::exists(iter_cur->path)){
				share::singleton().erase(iter_cur->path);
				{//begin lock scope
				boost::mutex::scoped_lock lock(job_queue_mutex);
				boost::shared_ptr<file_info> FI(new file_info(*iter_cur));
				FI->file_size = 0;
				job_queue.push_back(FI);
				job_queue_cond.notify_one();
				}//end lock scope
				skip_sleep = true;
			}
		}
	}
}
