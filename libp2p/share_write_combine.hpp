//THREADSAFE, CTOR THREAD SPAWNING
/*
This class is designed the encapsulate a thread which combines updating the
share and hash table in to large transactions.

Rationale: Updates to the share and hash table can be very frequent when
large directories are removed or directories with lots of small files are added.
Without combining the updates in to large transactions there is an unreasonable
amount of database contention.
*/

//custom
#include "share_scan.hpp"

class share_write_combine
{
public:
	share_write_combine();

private:
	boost::thread write_combine_thread;

	//jobs that will be combined in to a transaction
	std::vector<share_scan::job_info> write_combine_job;
	boost::mutex write_combine_job_mutex;

	void main_loop();
};
