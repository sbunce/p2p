#include "share_scan_1_hash.hpp"

share_scan_1_hash::share_scan_1_hash(
	shared_files & Shared_Files_in
):
	Shared_Files(Shared_Files_in),
	Share_Scan_0_Scan(Shared_Files)
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
	while(job_queue.size() >= settings::SHARE_SCAN_RATE){
		job_queue_max_cond.wait(job_queue_mutex);
	}
}

boost::shared_ptr<share_scan_job> share_scan_1_hash::job()
{
	boost::mutex::scoped_lock lock(job_queue_mutex);
	if(job_queue.empty()){
		return boost::shared_ptr<share_scan_job>();
	}else{
		boost::shared_ptr<share_scan_job> SPJ = job_queue.front();
		job_queue.pop_front();
		job_queue_max_cond.notify_all();
		return SPJ;
	}
}

void share_scan_1_hash::main_loop()
{
	//yield to other threads during prorgram start
	boost::this_thread::yield();

	while(true){
		boost::this_thread::interruption_point();
		block_on_max_jobs();
		boost::shared_ptr<share_scan_job> SPJ = Share_Scan_0_Scan.job();
		if(SPJ->add){
			if(hash_tree::create(SPJ->File)){
				Shared_Files.insert_update(SPJ->File);
				{//begin lock scope
				boost::mutex::scoped_lock lock(job_queue_mutex);
				job_queue.push_back(SPJ);
				}//end lock scope
			}else{
				Shared_Files.erase(SPJ->File.path);
			}
		}else{
			//file needs to be removed from database
			boost::mutex::scoped_lock lock(job_queue_mutex);
			job_queue.push_back(SPJ);
		}
	}
}
