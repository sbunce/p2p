#include "share_scan_0_scan.hpp"

share_scan_0_scan::share_scan_0_scan(
	shared_files & Shared_Files_in
):
	Shared_Files(Shared_Files_in)
{
	scan_thread = boost::thread(boost::bind(&share_scan_0_scan::main_loop, this));
}

share_scan_0_scan::~share_scan_0_scan()
{
	scan_thread.interrupt();
	scan_thread.join();
}

void share_scan_0_scan::block_on_max_jobs()
{
	boost::mutex::scoped_lock lock(job_queue_mutex);
	while(job_queue.size() >= settings::SHARE_SCAN_RATE){
		job_queue_max_cond.wait(job_queue_mutex);
	}
}

boost::shared_ptr<share_scan_job> share_scan_0_scan::job()
{
	boost::mutex::scoped_lock lock(job_queue_mutex);
	while(job_queue.empty()){
		job_queue_cond.wait(job_queue_mutex);
	}
	boost::shared_ptr<share_scan_job> SSJ = job_queue.front();
	job_queue.pop_front();
	job_queue_max_cond.notify_one();
	return SSJ;
}

int resume_call_back(shared_files & Shared_Files, int columns_retrieved, char ** response,
	char ** column_name)
{
	assert(response[0] && response[1] && response[2]);
	namespace fs = boost::filesystem;

	//path
	fs::path path = fs::system_complete(fs::path(response[1], fs::native));

	//file size
	boost::uint64_t file_size;
	std::stringstream ss;
	ss << response[2];
	ss >> file_size;

	Shared_Files.insert_update(shared_files::file(response[0], path.string(), file_size));

	if(boost::this_thread::interruption_requested()){
		return 1;
	}else{
		return 0;
	}
}

void share_scan_0_scan::main_loop()
{
	//yield to other threads during prorgram start
	boost::this_thread::yield();

	//read in existing share information from database
	database::pool::get_proxy()->query(
		"SELECT hash, path, file_size FROM share WHERE state = 1",
		&resume_call_back, Shared_Files);

	boost::filesystem::path share_path(boost::filesystem::system_complete(
		boost::filesystem::path(path::share(), boost::filesystem::native)));
	boost::posix_time::milliseconds scan_delay(1000 / settings::SHARE_SCAN_RATE);

	while(true){
		boost::this_thread::interruption_point();
		boost::this_thread::sleep(scan_delay);
		try{
			//scan share
			boost::filesystem::recursive_directory_iterator iter_cur(share_path), iter_end;
			while(iter_cur != iter_end){
				boost::this_thread::interruption_point();
				boost::this_thread::sleep(scan_delay);
				block_on_max_jobs();
				if(boost::filesystem::is_symlink(iter_cur->path().parent_path())){
					//traversed to symlink directory, go back up and skip
					iter_cur.pop();
				}else{
					if(boost::filesystem::is_regular_file(iter_cur->status())){
						std::string path = iter_cur->path().string();
						boost::uint64_t file_size = boost::filesystem::file_size(path);
						boost::shared_ptr<const shared_files::file> F = Shared_Files.lookup_path(path);
						if(!F || F->file_size != file_size){
							//file hasn't been seen before, or it changed size
							boost::shared_ptr<share_scan_job> SSJ(
								new share_scan_job(true, shared_files::file("", path, file_size)));
							Shared_Files.insert_update(SSJ->File);

							{//begin lock scope
							boost::mutex::scoped_lock lock(job_queue_mutex);
							job_queue.push_back(SSJ);
							job_queue_cond.notify_one();
							}//end lock scope
						}
					}
					++iter_cur;
				}
			}
		}catch(std::exception & ex){
			//LOGGER << ex.what();
		}

		//remove missing
		for(shared_files::const_iterator iter_cur = Shared_Files.begin(),
			iter_end = Shared_Files.end(); iter_cur != iter_end; ++iter_cur)
		{
			boost::this_thread::interruption_point();
			boost::this_thread::sleep(scan_delay);
			if(!boost::filesystem::exists(iter_cur->path)){
				//erasing from Shared_Files doesn't invalidate iterator
				Shared_Files.erase(iter_cur);
				boost::shared_ptr<share_scan_job> SSJ(
					new share_scan_job(false, *iter_cur));
				{//begin lock scope
				boost::mutex::scoped_lock lock(job_queue_mutex);
				job_queue.push_back(SSJ);
				job_queue_cond.notify_one();
				}//end lock scope
			}
		}
	}
}
