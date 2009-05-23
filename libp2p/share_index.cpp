#include "share_index.hpp"

share_index::share_index():
	DB(path::database()),
	workers_hashing(0)
{
	remove_temporary_files();

	//start one thread per core
	for(int x=0; x<boost::thread::hardware_concurrency(); ++x){
		Workers.create_thread(boost::bind(&share_index::worker_pool, this));
	}
}

bool share_index::is_indexing()
{
	return workers_hashing;
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
	std::string root_hash;
	database::connection DB(path::database());
	while(true){
		boost::this_thread::interruption_point();

		//wait for the scanner to give worker a job
		Share_Scan.get_job(info);
		++workers_hashing;

		if(info.add){
			/*
			If hash_tree::create returns false it means there was a read error or that
			hash tree generation was stopped because program is being shut down.
			*/
			if(Hash_Tree.create(info.path, info.size, root_hash)){
				LOGGER << "add: " << info.path << " size: " << info.size;
				database::table::share::add_entry(root_hash, info.size, info.path, DB);
				Share_Scan.share_size_add(info.size);
			}
		}else{
			LOGGER << "del: " << info.path;
			database::table::share::delete_entry(info.path, DB);
			Share_Scan.share_size_sub(info.size);
		}
		--workers_hashing;
	}
}
