#include "share_index.hpp"

share_index::share_index()
{
	remove_temporary_files();

	//start one thread per core
	int worker_count;
	if(boost::thread::hardware_concurrency() > 8){
		worker_count = 8;
	}else{
		worker_count = boost::thread::hardware_concurrency();
	}

	for(int x=0; x<worker_count; ++x){
		Workers.create_thread(boost::bind(&share_index::worker_pool, this));
	}
	write_combine_thread = boost::thread(boost::bind(&share_index::write_combine, this));
}

void share_index::remove_temporary_files()
{
	namespace fs = boost::filesystem;

	boost::uint64_t size;
	fs::path path = fs::system_complete(fs::path(path::temp(), fs::native));
	if(fs::exists(path)){
		try{
			fs::directory_iterator iter_cur(path), iter_end;
			while(iter_cur != iter_end){
				if(iter_cur->path().filename().find("rightside_up_", 0) == 0
					|| iter_cur->path().filename().find("upside_down_", 0) == 0)
				{
					fs::remove(iter_cur->path());
				}
				++iter_cur;
			}
		}catch(std::exception & ex){
			LOGGER << ex.what();
		}
	}
}

boost::uint64_t share_index::share_size()
{
	return Share_Scan.share_size();
}

void share_index::stop()
{
	Share_Scan.stop();
	Workers.interrupt_all();
	Workers.join_all();
	remove_temporary_files();
}

void share_index::worker_pool()
{
	hash_tree Hash_Tree(path::database());
	share_scan::job_info info;
	while(true){
		boost::this_thread::interruption_point();

		//wait for the scanner to give worker a job
		Share_Scan.get_job(info);

		if(info.add){
			/*
			If hash_tree::create returns false it means there was a read error or that
			hash tree generation was stopped because program is being shut down.
			*/
			if(Hash_Tree.create(info.path, info.file_size, info.hash)){
				boost::mutex::scoped_lock lock(write_combine_job_mutex);
				//LOGGER << "add: " << info.path << " size: " << info.file_size;
				write_combine_job.push_back(info);
				Share_Scan.share_size_add(info.file_size);
			}
		}else{
			boost::mutex::scoped_lock lock(write_combine_job_mutex);
			//LOGGER << "del: " << info.path;
			write_combine_job.push_back(info);
			Share_Scan.share_size_sub(info.file_size);
		}
	}
}

//DEBUG, put write combine in it's own class?
void share_index::write_combine()
{
	database::connection DB(path::database());
	std::vector<share_scan::job_info> temp;
	while(true){
		if(boost::this_thread::interruption_requested()){
			/*
			Make sure all jobs written to database when thread terminated.
			*/
			boost::mutex::scoped_lock lock(write_combine_job_mutex);
			if(write_combine_job.empty()){
				break;
			}
		}else{
			//write database at this interval + previous time to write database
			portable_sleep::ms(1000);
		}

		/*
		Copy the write_combine_job vector so we don't have to hold the lock to it
		while we iterate through the jobs and do queries.
		*/
		{//begin lock scope
		boost::mutex::scoped_lock lock(write_combine_job_mutex);
		temp.assign(write_combine_job.begin(), write_combine_job.end());
		write_combine_job.clear();
		}//end lock scope

		DB.query("BEGIN TRANSACTION");
		std::vector<share_scan::job_info>::iterator iter_cur = temp.begin(),
			iter_end = temp.end();
		while(iter_cur != iter_end){
			if(iter_cur->add){
				//job to add file to share table
				database::table::share::add_entry(iter_cur->hash, iter_cur->file_size, iter_cur->path, DB);
				database::table::hash::set_state(iter_cur->hash,
					hash_tree::tree_info::file_size_to_tree_size(iter_cur->file_size),
					database::table::hash::COMPLETE, DB);
			}else{
				//job to remove file from share table
				database::table::share::delete_entry(iter_cur->path, DB);
			}
			++iter_cur;
		}
		DB.query("END TRANSACTION");
	}
}
