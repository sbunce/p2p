#include "share_scan_1_hash.hpp"

share_scan_1_hash::share_scan_1_hash(
	share & Share_in
):
	Share(Share_in),
	Share_Scan_0_Scan(Share)
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
		std::string path = FI->path;

		//memoize the paths so that threads don't concurrently hash the same file
		{//begin lock scope
		boost::mutex::scoped_lock lock(memoize_mutex);
		std::pair<std::set<std::string>::iterator, bool> ret = memoize.insert(path);
		if(ret.second == false){
			//file is already hashing
			continue;
		}
		}//end lock scope

		if(FI->file_size != 0){
			hash_tree HT(*FI);
			hash_tree::status S = HT.create();
			FI->hash = HT.hash;
			if(S == hash_tree::good){
				Share.insert_update(*FI);
				{//begin lock scope
				boost::mutex::scoped_lock lock(job_queue_mutex);
				job_queue.push_back(FI);
				}//end lock scope
			}else{
				//error hashing, remove it and retry later
				Share.erase(FI->path);
			}
		}else{
			//file needs to be removed from database
			boost::mutex::scoped_lock lock(job_queue_mutex);
			job_queue.push_back(FI);
		}

		//unmemoize the file
		{//begin lock scope
		boost::mutex::scoped_lock lock(memoize_mutex);
		memoize.erase(path);
		}//end lock scope
	}
}
