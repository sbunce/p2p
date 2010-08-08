#include "share_scanner.hpp"

share_scanner::share_scanner(connection_manager & Connection_Manager_in):
	Connection_Manager(Connection_Manager_in),
	started(false),
	Thread_Pool(1)
{
	//default share
	shared.push_back(path::share_dir());
}

share_scanner::~share_scanner()
{
	hash_tree::stop_create();
}

void share_scanner::hash_file(boost::filesystem::recursive_directory_iterator it)
{
	if(share::singleton()->find_path(it->path().string()) == share::singleton()->end_file()){
		file_info FI;
		FI.path = it->path().string();
		try{
			FI.file_size = boost::filesystem::file_size(it->path());
			FI.last_write_time = boost::filesystem::last_write_time(it->path());
		}catch(const std::exception & e){
			LOG << e.what();
			start_scan();
			return;
		}
		hash_tree::status Status = hash_tree::create(FI);
		if(Status == hash_tree::good){
			share::singleton()->insert(FI);
			db::table::share::add(db::table::share::info(FI.hash, FI.path,
				FI.file_size, FI.last_write_time, db::table::share::complete));
			db::table::hash::set_state(FI.hash, db::table::hash::complete);
			Connection_Manager.add(FI.hash);
		}else{
			share::singleton()->erase(FI.path);
		}
	}
	Thread_Pool.enqueue(boost::bind(&share_scanner::scan, this, ++it));
}

void share_scanner::remove_missing(share::const_file_iterator it)
{
	if(it == share::singleton()->end_file()){
		//finished scanning existing files
		start_scan();
	}else{
		//check if exists
		if(!boost::filesystem::exists(it->path)
			&& !share::singleton()->is_downloading(it->path))
		{
			share::singleton()->erase(it->path);
			db::table::share::remove(it->path);
		}
		Thread_Pool.enqueue(boost::bind(&share_scanner::remove_missing, this, ++it), scan_delay_ms);
	}
}

void share_scanner::scan(boost::filesystem::recursive_directory_iterator it)
{
	try{
		if(it == boost::filesystem::recursive_directory_iterator()){
			//finished scanning share, remove missing
			Thread_Pool.enqueue(boost::bind(&share_scanner::remove_missing, this,
				share::singleton()->begin_file()), scan_delay_ms);
		}else if(boost::filesystem::is_symlink(it->path().parent_path())){
			//do not follow symlinks to avoid infinite loops
			it.pop();
			Thread_Pool.enqueue(boost::bind(&share_scanner::scan, this, it), scan_delay_ms);
		}else if(boost::filesystem::is_regular_file(it->status())){
			//potential file to hash
			boost::uint64_t file_size = boost::filesystem::file_size(it->path());
			std::time_t last_write_time = boost::filesystem::last_write_time(it->path());
			share::const_file_iterator share_it = share::singleton()->find_path(it->path().string());
			bool exists_in_share = share_it != share::singleton()->end_file();
			bool downloading = share::singleton()->is_downloading(it->path().string());
			bool modified = share_it == share::singleton()->end_file()
				|| share_it->file_size != file_size
				|| share_it->last_write_time != last_write_time;
			//file may be copying if it was recently modified
			bool recently_modified = std::time(NULL) - last_write_time < 8;
			if((!exists_in_share && !recently_modified)
				|| (!downloading && modified && !recently_modified))
			{
				Thread_Pool.enqueue(boost::bind(boost::bind(&share_scanner::hash_file, this, it)));
			}else{
				Thread_Pool.enqueue(boost::bind(&share_scanner::scan, this, ++it), scan_delay_ms);
			}
		}else{
			//not valid file to hash
			Thread_Pool.enqueue(boost::bind(&share_scanner::scan, this, ++it), scan_delay_ms);
		}
	}catch(const std::exception & e){
		LOG << e.what();
		start_scan();
	}
}

void share_scanner::start()
{
	boost::mutex::scoped_lock lock(start_mutex);
	if(!started){
		start_scan();
		started = true;
	}
}

void share_scanner::start_scan()
{
	boost::mutex::scoped_lock lock(shared_mutex);
	if(shared.empty()){
		//no shared directories
		Thread_Pool.enqueue(boost::bind(&share_scanner::start_scan, this), scan_delay_ms);
		return;
	}
	std::string start = shared.front();
	do{
		shared.push_back(shared.front());
		shared.pop_front();
		try{
			boost::filesystem::recursive_directory_iterator it(shared.front());
			Thread_Pool.enqueue(boost::bind(&share_scanner::scan, this, it), scan_delay_ms);
			return;
		}catch(const std::exception & e){
			LOG << e.what();
		}
	}while(shared.front() != start);

	//no shared directory exists
	Thread_Pool.enqueue(boost::bind(&share_scanner::start_scan, this), scan_delay_ms);
}
