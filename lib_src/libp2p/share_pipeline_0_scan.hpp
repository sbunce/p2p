#ifndef H_SHARE_PIPELINE_0_SCAN
#define H_SHARE_PIPELINE_0_SCAN

//custom
#include "database.hpp"
#include "path.hpp"
#include "settings.hpp"
#include "share_pipeline_job.hpp"

//include
#include <atomic_int.hpp>
#include <boost/cstdint.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/tokenizer.hpp>
#include <logger.hpp>

//standard
#include <cassert>
#include <deque>
#include <map>
#include <string>

class share_pipeline_0_scan : private boost::noncopyable
{
public:
	share_pipeline_0_scan(
		atomic_int<boost::uint64_t> & _size_bytes_in,
		atomic_int<boost::uint64_t> & _size_files_in
	);
	~share_pipeline_0_scan();

	/*
	get_job:
		The share_pipeline_1_hash threads get blocked at this function until there
		is work to do.
	*/
	void get_job(share_pipeline_job & info);
	boost::uint64_t size_bytes();
	boost::uint64_t size_files();

private:
	boost::thread scan_thread;

	//jobs for share_pipeline_1_scan process
	std::deque<share_pipeline_job> job;
	boost::mutex job_mutex;
	boost::condition_variable_any job_cond;

	//used to block scan_thread when job limit reached
	boost::condition_variable_any job_max_cond;

	//file node in tree
	class file
	{
	public:
		file(const boost::uint64_t & size_in): size(size_in){}
		boost::uint64_t size;
	};

	//directory node in tree
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

	//path to start recursive scan
	boost::filesystem::path share_path;

	//the size of all files and the number of files in the share
	atomic_int<boost::uint64_t> & _size_bytes;
	atomic_int<boost::uint64_t> & _size_files;

	/*
	block_on_max_jobs:
		Blocks the scan thread when the job buffer is full. This function will
		unblock when share_pipeline_1_hash takes some of the finished jobs using
		get_job().
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
#endif
