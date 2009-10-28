#include "share_scan_1_hash.hpp"

share_scan_1_hash::share_scan_1_hash()
{
	for(int x=0; x<boost::thread::hardware_concurrency(); ++x){
		Workers.create_thread(boost::bind(&share_scan_1_hash::main_loop, this));
	}
}

share_scan_1_hash::~share_scan_1_hash()
{
	Workers.interrupt_all();
	Workers.join_all();
}

void share_scan_1_hash::block_on_max_jobs()
{
	boost::mutex::scoped_lock lock(job_queue_mutex);
	while(job_queue.size() >= settings::SHARE_BUFFER_SIZE){
		job_queue_max_cond.wait(job_queue_mutex);
	}
}

void share_scan_1_hash::block_until_resumed()
{
	Share_Scan_0_Scan.block_until_resumed();
}

boost::shared_ptr<file_info> share_scan_1_hash::job()
{
	boost::mutex::scoped_lock lock(job_queue_mutex);
	if(job_queue.empty()){
		return boost::shared_ptr<file_info>();
	}else{
		boost::shared_ptr<file_info> FI = job_queue.front();
		job_queue.pop_front();
		job_queue_max_cond.notify_all();
		return FI;
	}
}

void share_scan_1_hash::main_loop()
{
	while(true){
		boost::this_thread::interruption_point();
		block_on_max_jobs();
		boost::shared_ptr<file_info> FI = Share_Scan_0_Scan.job();

		//memoize the paths so that threads don't concurrently hash the same file
		{//begin lock scope
		boost::mutex::scoped_lock lock(memoize_mutex);
		std::pair<std::map<std::string, std::time_t>::iterator, bool>
			ret = memoize.insert(std::make_pair(FI->path, 0));
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

		hash_tree::status Status;
		if(FI->file_size != 0){
			hash_tree HT(*FI);
			Status = HT.create();
			FI->hash = HT.hash;
			if(Status == hash_tree::good){
				share::singleton().insert_update(*FI);
				{//begin lock scope
				boost::mutex::scoped_lock lock(job_queue_mutex);
				job_queue.push_back(FI);
				}//end lock scope
			}else{
				//error hashing, remove it and retry later
				share::singleton().erase(FI->path);
			}
		}else{
			//file needs to be removed from database
			boost::mutex::scoped_lock lock(job_queue_mutex);
			job_queue.push_back(FI);
		}

		//unmemoize the file
		{//begin lock scope
		boost::mutex::scoped_lock lock(memoize_mutex);
		if(Status == hash_tree::copying){
			//don't attempt to hash this file again for 1 minute
			std::map<std::string, std::time_t>::iterator iter = memoize.find(FI->path);
			assert(iter != memoize.end());
			iter->second = std::time(NULL) + 60;
		}else{
			memoize.erase(FI->path);
		}
		}//end lock scope
	}
}
