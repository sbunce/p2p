#include "share_pipeline_1_hash.hpp"

share_pipeline_1_hash::share_pipeline_1_hash()
{

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
	//delay allows p2p_real to be constructed faster
	portable_sleep::ms(100);

	hash_tree Hash_Tree;
	share_pipeline_job info;
	while(true){
		boost::this_thread::interruption_point();

		//block if job buffer full
		block_on_max_jobs();

		//wait for the scanner to give worker a job
		Share_Pipeline_0_Scan.get_job(info);

		if(info.add){
			/*
			If hash_tree::create returns false it means there was a read error or
			that hash tree generation was stopped because program is being shut
			down.
			*/
			if(Hash_Tree.create(info.path, info.file_size, info.hash)){
				boost::mutex::scoped_lock lock(job_mutex);
				job.push_back(info);
			}
		}else{
			boost::mutex::scoped_lock lock(job_mutex);
			job.push_back(info);
		}
	}
}

boost::uint64_t share_pipeline_1_hash::size_bytes()
{
	return Share_Pipeline_0_Scan.size_bytes();
}

boost::uint64_t share_pipeline_1_hash::size_files()
{
	return Share_Pipeline_0_Scan.size_files();
}

void share_pipeline_1_hash::start()
{
	for(int x=0; x<boost::thread::hardware_concurrency(); ++x){
		Workers.create_thread(boost::bind(&share_pipeline_1_hash::main_loop, this));
	}
	Share_Pipeline_0_Scan.start();
}

void share_pipeline_1_hash::stop()
{
	Share_Pipeline_0_Scan.stop();
	Workers.interrupt_all();
	Workers.join_all();
}
