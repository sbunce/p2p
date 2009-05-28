//THREAD-SAFE, CTOR THREAD SPAWNING
/*
This tree saves size when storing directory paths because it represents the
paths as a tree. If there are two files in a directory the directory name is
only stored once.

Empty directories are not tracked.

All data structures private to this class should only be accessed by scan_thread
unless properly synchronized (like the job dequeue).
*/

//custom
#include "database.hpp"
#include "path.hpp"
#include "settings.hpp"

//boost
#include <boost/cstdint.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/tokenizer.hpp>

//include
#include <atomic_int.hpp>
#include <logger.hpp>
#include <portable_sleep.hpp>

//std
#include <cassert>
#include <deque>
#include <map>
#include <string>

class share_scan : private boost::noncopyable
{
public:
	share_scan();

	class job_info
	{
		friend class share_scan;
	public:
		job_info(){}

		std::string path;
		boost::uint64_t file_size;

		//true if file added, false if removed
		bool add;

		//used by share_index
		std::string hash;

	private:
		job_info(
			const std::string & path_in,
			const boost::uint64_t & file_size_in,
			const bool add_in
		):
			path(path_in),
			file_size(file_size_in),
			add(add_in)
		{}
	};

	/*
	get_job:
		This is called from multiple threads concurrently. Blocks until a job is
		read (hash tree needs to be created).
		std::pair<file path, file_size>
	share_size:
		Returns the size of all shared files.
	share_size_add:
		share_index adds bytes to share size when it finishes hashing a file.
	share_size_sub:
		share_index subtracts bytes from share size when it removes a file from
		the DB.
	stop:
		Stops the share_scan thread. Called by stop function of share_index. This
		is needed because singletons don't get destructred. So cleanup and thread
		shutdown must be done manually.
	*/
	void get_job(job_info & info);
	boost::uint64_t share_size();
	void share_size_add(const boost::uint64_t & bytes);
	void share_size_sub(const boost::uint64_t & bytes);
	void stop();

private:
	boost::thread scan_thread;

	//information to be stored for a file
	class file
	{
	public:
		file(const boost::uint64_t & size_in): size(size_in){}
		boost::uint64_t size;
	};

	class directory
	{
	public:
		std::map<std::string, directory> Directory;
		std::map<std::string, file> File;
	};

	/*
	The filesystem root starts at root.Directory.begin(). On windows there can be
	more than one filesystem root. In which case root.Directory.size() > 1. On
	POSIX systems root.Directory.size() == 1. The File data member will always be
	empty in the root node.

	If the tree is empty then root.Directory.empty() == true.
	*/
	directory root;

//DEBUG, eventually this should be a container of paths to scan
	//path to scan
	boost::filesystem::path share_path;

	//the sum of the size of all files that have been processed (bytes)
	atomic_int<boost::uint64_t> _share_size;

	/*
	Jobs for share_index worker threads to pick up.
	*/
	std::deque<job_info> job;
	boost::mutex job_mutex;
	boost::condition_variable_any job_cond;

	//used to block scan_thread when job limit reached
	boost::condition_variable_any job_max_cond;

	/*
	block_on_max_jobs:
		Blocks the scan thread when the maximum number of jobs has been reached.
	insert and insert_recurse:
		Adds file to tree. Returns true if a job for the file needs to be
		scheduled.
	main_loop:
		Function scan_thread operates in.
	remove_missing and remove_missing_recurse:
		Traverses entire tree and checks if files exist. If a file doesn't exist
		a job to remove the file from the database is scheduled.
	*/
	void block_on_max_jobs();
	bool insert(const std::string & path, const boost::uint64_t & size);
	bool insert_recurse(std::map<std::string, directory>::iterator dir_iter,
		boost::tokenizer<boost::char_separator<char> >::iterator path_iter_cur,
		boost::tokenizer<boost::char_separator<char> >::iterator path_iter_end,
		const boost::uint64_t & size);
	void main_loop();
	void remove_missing();
	void remove_missing_recurse(std::map<std::string, directory>::iterator dir_iter,
		std::string constructed_path);

	//call back used by ctor to populate tree with known files
	int path_call_back(int columns_retrieved, char ** response, char ** column_name);
};
