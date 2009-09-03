#include "share_pipeline_1_hash.hpp"

share_pipeline_1_hash::share_pipeline_1_hash():
	_size_bytes(0),
	_size_files(0),
	Share_Pipeline_0_Scan(_size_bytes, _size_files)
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
	/*
	Thread takes a lot of CPU time to bring up. This can increase the time it
	takes to instantiate libp2p. This delay speeds up library instantiation. This
	is important for fast GUI startup time.
	*/
	boost::this_thread::yield();

	share_pipeline_job SPJ;
	while(true){
		boost::this_thread::interruption_point();

		//block if job buffer full
		block_on_max_jobs();

		//wait for the scanner to give worker a job
		Share_Pipeline_0_Scan.get_job(SPJ);

		/*
		Process different jobs types. We update the _size_bytes and _size_files
		counts here because we only want to update the counts after hashing is
		done and we don't want to do it in share_pipeline_2 because that sleeps
		for most of every second which would make the count updates look "skippy".
		*/
		if(SPJ.Type == share_pipeline_job::HASH_FILE){
			/*
			New file discovered which needs to be hashed. If hash_tree::create
			returns false there was a error and we drop the job.
			*/
			if(hash_tree::create(SPJ.path, SPJ.file_size, SPJ.hash)){
				boost::mutex::scoped_lock lock(job_mutex);
				job.push_back(SPJ);
			}
			_size_bytes += SPJ.file_size;
			++_size_files;
		}else if(SPJ.Type == share_pipeline_job::REMOVE_FILE){
			/*
			File needs to be removed from the database, we update counts and pass
			the jobs on to stage 2 for updating the database.
			*/
			boost::mutex::scoped_lock lock(job_mutex);
			job.push_back(SPJ);
			_size_bytes -= SPJ.file_size;
			--_size_files;
		}else{
			LOGGER << "programmer error";
			exit(1);
		}
	}
}

boost::uint64_t share_pipeline_1_hash::size_bytes()
{
	/*
	If a file is added to the share and removed before the hash tree is created
	then _size_bytes can momentarily drop below zero (since unsigned it wraps to
	a really high number). This is a workaround to not display those high values.
	*/
	return _size_bytes > 0ULL - 4294967296ULL ?
		atomic_int<boost::uint64_t>(0) : _size_bytes;
}

boost::uint64_t share_pipeline_1_hash::size_files()
{
	return _size_files > 0ULL - 4294967296ULL ?
		atomic_int<boost::uint64_t>(0) : _size_files;
}
