#include "share_pipeline_1_hash.hpp"

share_pipeline_1_hash::share_pipeline_1_hash()
{
	//start one thread per hardware thread with a max of 8
	int worker_count;
	if(boost::thread::hardware_concurrency() > 8){
		worker_count = 8;
	}else{
		worker_count = boost::thread::hardware_concurrency();
	}

	for(int x=0; x<worker_count; ++x){
		Workers.create_thread(boost::bind(&share_pipeline_1_hash::main_loop, this));
	}
}

void share_pipeline_1_hash::block_on_max_jobs()
{
	boost::mutex::scoped_lock lock(job_mutex);
	while(job.size() >= settings::SHARE_SCAN_RATE){
		job_max_cond.wait(job_mutex);
	}
}

bool share_pipeline_1_hash::get_job(share_pipeline_job & info)
{
	boost::mutex::scoped_lock lock(job_mutex);
	if(job.empty()){
		return false;
	}else{
		info = job.front();
		job.pop_front();
		job_max_cond.notify_all();
		return true;
	}
}

void share_pipeline_1_hash::main_loop()
{
	hash_tree Hash_Tree(path::database());
	share_pipeline_job info;
	while(true){
		boost::this_thread::interruption_point();

		//block if job buffer full
		block_on_max_jobs();

		//wait for the scanner to give worker a job
		Share_Pipeline_0_Scan.get_job(info);

		if(info.add){
			/*
			If hash_tree::create returns false it means there was a read error or that
			hash tree generation was stopped because program is being shut down.
			*/
			if(Hash_Tree.create(info.path, info.file_size, info.hash)){
				boost::mutex::scoped_lock lock(job_mutex);
				//LOGGER << "add: " << info.path << " size: " << info.file_size;
				job.push_back(info);
			}
		}else{
			boost::mutex::scoped_lock lock(job_mutex);
			//LOGGER << "del: " << info.path;
			job.push_back(info);
		}
	}
}

boost::uint64_t share_pipeline_1_hash::share_size()
{
	return Share_Pipeline_0_Scan.share_size();
}

void share_pipeline_1_hash::stop()
{
	Share_Pipeline_0_Scan.stop();
	Workers.interrupt_all();
	Workers.join_all();
}
