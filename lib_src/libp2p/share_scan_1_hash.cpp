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
	while(job_queue.size() >= settings::SHARE_SCAN_RATE){
		job_queue_max_cond.wait(job_queue_mutex);
	}
}

void share_scan_1_hash::block_until_resumed()
{
	Share_Scan_0_Scan.block_until_resumed();
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
			hash_tree::status S = hash_tree::create(SPJ->FI);
			if(S == hash_tree::good){
				SPJ->FI.complete = true;
				Share.insert_update(SPJ->FI);
				{//begin lock scope
				boost::mutex::scoped_lock lock(job_queue_mutex);
				job_queue.push_back(SPJ);
				}//end lock scope
			}else if(S == hash_tree::io_error){
				//io_error, remove file from share so it tries to rehash later
				Share.erase(SPJ->FI.path);
			}else if(S == hash_tree::bad){
				/*
				Tree can never be created. We leave the hash for the file in share
				set to "" so that we don't try to rehash this file again and so that
				it doesn't count to the file or byte total for the share.
				*/
				LOGGER << "bad file: " << SPJ->FI.path;
				SPJ->FI.hash = "";
				SPJ->FI.complete = true;
				Share.insert_update(SPJ->FI);
			}
		}else{
			//file needs to be removed from database
			boost::mutex::scoped_lock lock(job_queue_mutex);
			job_queue.push_back(SPJ);
		}
	}
}
