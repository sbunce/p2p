//THREADSAFE, CTOR THREAD SPAWNING
/*
This class is designed the encapsulate a thread which combines updating the
share and hash table in to large transactions.

Rationale: Updates to the share and hash table can be very frequent when
large directories are removed or directories with lots of small files are added.
Without combining the updates in to large transactions there is an unreasonable
amount of database contention.
*/
#ifndef H_SHARE_PIPELINE_2_WRITE
#define H_SHARE_PIPELINE_2_WRITE

//custom
#include "share_pipeline_job.hpp"
#include "share_pipeline_1_hash.hpp"

//include
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <portable_sleep.hpp>

//standard
#include <vector>

class share_pipeline_2_write
{
public:
	share_pipeline_2_write();

	/*
	start:
		Starts write_thread which combines writes to the database.
	stop:
		Stops write_thread.
	*/
	void start();
	void stop();

	/*
	stop:
		Stops all pipeline threads starting with pipeline 0 and working it's way
		back up.
	*/
	boost::uint64_t size_bytes();
	boost::uint64_t size_files();

private:
	boost::thread write_thread;
	
	/*
	main_loop:
		The thread which combines database writes in to transactions waits in this
		thread.
	*/
	void main_loop();

	//the stage before this stage
	share_pipeline_1_hash Share_Pipeline_1_Hash;
};
#endif
