#include "share_pipeline_1_hash.hpp"

share_pipeline_1_hash::share_pipeline_1_hash(
	share_info & Share_Info_in
):
	Share_Info(Share_Info_in),
	Share_Pipeline_0_Scan(Share_Info)
{
	for(int x=0; x<boost::thread::hardware_concurrency(); ++x){
		Workers.create_thread(boost::bind(&share_pipeline_1_hash::main_loop, this));
	}
}

share_pipeline_1_hash::~share_pipeline_1_hash()
{
	Workers.interrupt_all();
	Workers.join_all();
}

void share_pipeline_1_hash::block_on_max_jobs()
{
	boost::mutex::scoped_lock lock(job_queue_mutex);
	while(job_queue.size() >= settings::SHARE_SCAN_RATE){
		job_queue_max_cond.wait(job_queue_mutex);
	}
}

boost::shared_ptr<share_pipeline_job> share_pipeline_1_hash::job()
{
	boost::mutex::scoped_lock lock(job_queue_mutex);
	if(job_queue.empty()){
		return boost::shared_ptr<share_pipeline_job>();
	}else{
		boost::shared_ptr<share_pipeline_job> SPJ = job_queue.front();
		job_queue.pop_front();
		job_queue_max_cond.notify_all();
		return SPJ;
	}
}

void share_pipeline_1_hash::main_loop()
{
	//yield to other threads during prorgram start
	boost::this_thread::yield();

	while(true){
		boost::this_thread::interruption_point();
		block_on_max_jobs();

		//wait for the scanner to give worker a job
		boost::shared_ptr<share_pipeline_job> SPJ = Share_Pipeline_0_Scan.job();
		if(SPJ->add){
			if(hash_tree::create(SPJ->File.path, SPJ->File.file_size, SPJ->File.hash)){
				Share_Info.insert_update(SPJ->File);
				{//begin lock scope
				boost::mutex::scoped_lock lock(job_queue_mutex);
				job_queue.push_back(SPJ);
				}//end lock scope
			}else{
				Share_Info.erase(SPJ->File.path);
			}
		}else{
			//file needs to be removed from database
			boost::mutex::scoped_lock lock(job_queue_mutex);
			job_queue.push_back(SPJ);
		}
	}
}
