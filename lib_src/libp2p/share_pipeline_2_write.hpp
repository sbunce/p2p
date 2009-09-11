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
#include "share_info.hpp"
#include "share_pipeline_job.hpp"
#include "share_pipeline_1_hash.hpp"

//include
#include <boost/bind.hpp>
#include <boost/thread.hpp>

//standard
#include <vector>

class share_pipeline_2_write
{
public:
	share_pipeline_2_write(share_info & Share_Info_in);
	~share_pipeline_2_write();

private:
	boost::thread write_thread;

	//contains files in share
	share_info & Share_Info;

	//previous stage in pipeline
	share_pipeline_1_hash Share_Pipeline_1_Hash;

	/*
	main_loop:
		Database write combining.
	*/
	void main_loop();
};
#endif
