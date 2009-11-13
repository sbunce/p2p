#include "share_scanner.hpp"

share_scanner::share_scanner()
{
	//this thread spawns workers that hash files when it finishes resume
	Workers.create_thread(boost::bind(&share_scanner::scan_loop, this));
}

share_scanner::~share_scanner()
{
	Workers.interrupt_all();
	Workers.join_all();
}

void share_scanner::hash_loop()
{
	while(true){
		boost::this_thread::interruption_point();

		//wait for job
		file_info FI;
		{//begin lock scope
		boost::mutex::scoped_lock lock(job_queue_mutex);
		while(job_queue.empty()){
			job_queue_cond.wait(job_queue_mutex);
		}
		FI = job_queue.front();
		job_queue.pop_front();
		job_queue_max_cond.notify_one();
		}//end lock scope

		//memoize the paths so that threads don't concurrently hash the same file
		{//begin lock scope
		boost::mutex::scoped_lock lock(memoize_mutex);
		std::pair<std::map<std::string, std::time_t>::iterator, bool>
			ret = memoize.insert(std::make_pair(FI.path, 0));
		if(ret.second == false){
			if(ret.first->second == 0){
				//file in progress of hashing
				continue;
			}else{
				//timeout set, check if it expired
				if(ret.first->second < std::time(NULL)){
					//timeout expired, try to hash file
					ret.first->second = 0;
				}else{
					//timeout not expired, do not try to hash file
					continue;
				}
			}
		}
		}//end lock scope

		hash_tree HT(FI);
		hash_tree::status Status = HT.create();
		FI.hash = HT.hash;
		if(Status == hash_tree::good){
			share::singleton().insert(FI);
			database::table::share::add(database::table::share::info(FI,
				database::table::share::complete));
			database::table::hash_tree::set_state(FI.hash,
				database::table::hash_tree::complete);
		}else{
			//error hashing, retry later
			share::singleton().erase(FI.path);
		}

		//unmemoize the file
		{//begin lock scope
		boost::mutex::scoped_lock lock(memoize_mutex);
		if(Status == hash_tree::copying){
			//don't attempt to hash this file again for 1 minute
			std::map<std::string, std::time_t>::iterator iter = memoize.find(FI.path);
			assert(iter != memoize.end());
			iter->second = std::time(NULL) + 60;
		}else{
			memoize.erase(FI.path);
		}
		}//end lock scope
	}
}

void share_scanner::scan_loop()
{
	share::singleton().resume_block();

	//start workers to hash files
	for(int x=0; x<boost::thread::hardware_concurrency(); ++x){
		Workers.create_thread(boost::bind(&share_scanner::hash_loop, this));
	}

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

				//block if we've created too many jobs
				{//begin lock scope
				boost::mutex::scoped_lock lock(job_queue_mutex);
				while(job_queue.size() >= settings::SHARE_BUFFER_SIZE){
					job_queue_max_cond.wait(job_queue_mutex);
				}
				}//end lock scope

				if(boost::filesystem::is_symlink(iter_cur->path().parent_path())){
					//traversed to symlink directory, go back up and skip
					iter_cur.pop();
				}else{
					if(boost::filesystem::is_regular_file(iter_cur->status())){
						std::string path = iter_cur->path().string();
						boost::uint64_t file_size = boost::filesystem::file_size(path);
						std::time_t last_write_time = boost::filesystem::last_write_time(iter_cur->path());
						share::const_file_iterator share_iter = share::singleton().find_path(path);
						if(share_iter == share::singleton().end_file()
							|| (!share::singleton().is_downloading(path) && (share_iter->file_size != file_size
							|| share_iter->last_write_time != last_write_time)))
						{
							//new file, or modified file
							boost::mutex::scoped_lock lock(job_queue_mutex);
							job_queue.push_back(file_info("", path, file_size, last_write_time));
							job_queue_cond.notify_one();
							skip_sleep = true;
						}
					}
					++iter_cur;
				}
			}
		}catch(std::exception & ex){
			LOGGER << ex.what();
		}

		//remove missing files
		for(share::const_file_iterator iter_cur = share::singleton().begin_file(),
			iter_end = share::singleton().end_file(); iter_cur != iter_end; ++iter_cur)
		{
			boost::this_thread::interruption_point();
			if(skip_sleep){
				skip_sleep = false;
			}else{
				boost::this_thread::sleep(scan_delay);
			}
			if(!boost::filesystem::exists(iter_cur->path.get())){
				share::singleton().erase(iter_cur->path);
				database::table::share::remove(iter_cur->path);
				skip_sleep = true;
			}
		}
	}
}
