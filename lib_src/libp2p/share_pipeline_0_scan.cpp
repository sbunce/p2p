#include "share_pipeline_0_scan.hpp"

share_pipeline_0_scan::share_pipeline_0_scan():
	share_path(boost::filesystem::system_complete(
		boost::filesystem::path(path::share(), boost::filesystem::native)
	)),
	_size_bytes(0)
{
	scan_thread = boost::thread(boost::bind(&share_pipeline_0_scan::main_loop, this));
}

void share_pipeline_0_scan::block_on_max_jobs()
{
	boost::mutex::scoped_lock lock(job_mutex);
	while(job.size() >= settings::SHARE_SCAN_RATE){
		job_max_cond.wait(job_mutex);
	}
}

void share_pipeline_0_scan::get_job(share_pipeline_job & info)
{
	boost::mutex::scoped_lock lock(job_mutex);
	while(job.empty()){
		job_cond.wait(job_mutex);
	}
	info = job.front();
	job.pop_front();
	job_max_cond.notify_one();
}

bool share_pipeline_0_scan::insert(const std::string & path, const boost::uint64_t & size)
{
	boost::char_separator<char> sep("/");
	boost::tokenizer<boost::char_separator<char> > tokens(path, sep);
	boost::tokenizer<boost::char_separator<char> >::iterator
		path_iter_cur = tokens.begin(), path_iter_end = tokens.end();

	if(path_iter_cur == path_iter_end){
		/*
		Invalid file path. Possible reasons:
		1. The path contains no separators "ABC".
		2. The path is the root directory "/".
		*/
		LOGGER << "invalid path: " << path;
		return false;
	}

	//make sure root directory exists
	std::map<std::string, directory>::iterator dir_iter;
	if(!path.empty() && path[0] != '/'){
		//first token is root directory
		std::pair<std::map<std::string, directory>::iterator, bool>
			ret = root.Directory.insert(std::make_pair(*path_iter_cur++, directory()));
		dir_iter = ret.first;
	}else{
		//first token is non-root directory
		std::pair<std::map<std::string, directory>::iterator, bool>
			ret = root.Directory.insert(std::make_pair("", directory()));
		dir_iter = ret.first;
	}

	if(path_iter_cur == path_iter_end){
		/*
		Invalid file path. Possible reasons:
		1. Path is a drive root on windows "c:/".
		*/
		return false;
	}else{
		return insert_recurse(dir_iter, path_iter_cur, path_iter_end, size);
	}
}

bool share_pipeline_0_scan::insert_recurse(std::map<std::string, directory>::iterator dir_iter,
	boost::tokenizer<boost::char_separator<char> >::iterator path_iter_cur,
	boost::tokenizer<boost::char_separator<char> >::iterator path_iter_end,
	const boost::uint64_t & size)
{
	//tokenizer has forward iterator, tmp needed to know if at last token
	boost::tokenizer<boost::char_separator<char> >::iterator tmp = path_iter_cur;
	++tmp;
	if(tmp == path_iter_end){
		std::pair<std::map<std::string, file>::iterator, bool>
			ret = dir_iter->second.File.insert(std::make_pair(*path_iter_cur, file(size)));
		return ret.second; //true if file didn't already exist
	}else{
		std::pair<std::map<std::string, directory>::iterator, bool>
			ret = dir_iter->second.Directory.insert(std::make_pair(*path_iter_cur, directory()));
		return insert_recurse(ret.first, ++path_iter_cur, path_iter_end, size);
	}
}

void share_pipeline_0_scan::main_loop()
{
	namespace fs = boost::filesystem;

	//populate in-memory tree from database
	database::pool::get_proxy()->query("SELECT path, size FROM share", this, &share_pipeline_0_scan::path_call_back);

	boost::uint64_t size;
	while(true){
		boost::this_thread::interruption_point();

		//traverse tree and remove all missing files
		remove_missing();

		if(!fs::exists(share_path)){
			LOGGER << "error opening share: " << share_path.string();
			portable_sleep::ms(1000);
			continue;
		}

		/*
		Scan thruogh all files in share and try to add them to the tree. If the
		file doesn't already exist in the tree then we schedule a job for the file
		to be hashed.
		*/
		try{
			fs::recursive_directory_iterator iter_cur(share_path), iter_end;
			while(iter_cur != iter_end){
				boost::this_thread::interruption_point();

				//scan rate
				portable_sleep::ms(1000 / settings::SHARE_SCAN_RATE);
				block_on_max_jobs();

				if(fs::is_symlink(iter_cur->path().parent_path())){
					//traversed to symlink directory, go back up and skip
					iter_cur.pop();
				}else{
					if(fs::is_regular_file(iter_cur->status())){
						size = fs::file_size(iter_cur->path().string());
						if(insert(iter_cur->path().string(), size)){
							//file hasn't been seen before, or it changed size
							boost::mutex::scoped_lock lock(job_mutex);
							_size_bytes += size;
							job.push_back(share_pipeline_job(iter_cur->path().string(), size, true));
							job_cond.notify_one();
						}
					}
					++iter_cur;
				}
			}
		}catch(std::exception & ex){
			LOGGER << ex.what();
		}

		//scan rate also needed here in case share empty
		portable_sleep::ms(1000 / settings::SHARE_SCAN_RATE);
	}
}

int share_pipeline_0_scan::path_call_back(int columns_retrieved, char ** response, char ** column_name)
{
	namespace fs = boost::filesystem;
	assert(response[0] && response[1]);
	boost::uint64_t size;
	std::stringstream ss;
	ss << response[1];
	ss >> size;
	_size_bytes += size;
	fs::path path = fs::system_complete(fs::path(response[0], fs::native));
	insert(path.string(), size);
	if(boost::this_thread::interruption_requested()){
		return 1;
	}else{
		return 0;
	}
}

void share_pipeline_0_scan::remove_missing()
{
	//recurse on all the different directory roots
	std::map<std::string, directory>::iterator
		iter_cur = root.Directory.begin(), iter_end = root.Directory.end();
	while(iter_cur != iter_end){
		std::string constructed_path;
		remove_missing_recurse(iter_cur, constructed_path);
		++iter_cur;
	}
}

void share_pipeline_0_scan::remove_missing_recurse(
	std::map<std::string, directory>::iterator dir_iter,
	std::string constructed_path)
{
	namespace fs = boost::filesystem;

	//traverse to sub-directories
	std::map<std::string, directory>::iterator
		dir_iter_cur = dir_iter->second.Directory.begin(),
		dir_iter_end = dir_iter->second.Directory.end();
	while(dir_iter_cur != dir_iter_end){
		boost::this_thread::interruption_point();
		remove_missing_recurse(dir_iter_cur, constructed_path + "/" + dir_iter_cur->first);
		if(dir_iter_cur->second.Directory.empty() && dir_iter_cur->second.File.empty()){
			//erase empty sub-directory
			dir_iter->second.Directory.erase(dir_iter_cur++);
		}else{
			++dir_iter_cur;
		}
	}

	//check files in directory
	std::map<std::string, file>::iterator
		file_iter_cur = dir_iter->second.File.begin(),
		file_iter_end = dir_iter->second.File.end();
	while(file_iter_cur != file_iter_end){
		boost::this_thread::interruption_point();
		std::string file_path = constructed_path + "/" + file_iter_cur->first;
		bool remove = false;
		try{
			//scan rate
			portable_sleep::ms(1000 / settings::SHARE_SCAN_RATE);
			block_on_max_jobs();

			fs::path path = fs::system_complete(fs::path(file_path, fs::native));

			bool exists_in_share = (path.string().find(share_path.string()) == 0u);
			if(!exists_in_share || !fs::exists(path)){
				/*
				File no longer exists in the share. This could be because the
				directory the file is in is no longer shared. Or the file was
				removed.
				*/
				boost::mutex::scoped_lock lock(job_mutex);
				_size_bytes -= file_iter_cur->second.size;
				job.push_back(share_pipeline_job(file_path, file_iter_cur->second.size, false));
				job_cond.notify_one();
				dir_iter->second.File.erase(file_iter_cur++);
				remove = true;
			}
		}catch(std::exception & ex){
			//error reading file, remove files that fail to read
			LOGGER << ex.what();
			boost::mutex::scoped_lock lock(job_mutex);
			_size_bytes -= file_iter_cur->second.size;
			job.push_back(share_pipeline_job(file_path, file_iter_cur->second.size, false));
			job_cond.notify_one();
			dir_iter->second.File.erase(file_iter_cur++);
			remove = true;
		}

		if(!remove){
			++file_iter_cur;
		}
	}
}

boost::uint64_t share_pipeline_0_scan::size_bytes()
{
	return _size_bytes;
}

void share_pipeline_0_scan::stop()
{
	scan_thread.interrupt();
	scan_thread.join();
}
