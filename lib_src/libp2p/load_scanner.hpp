#ifndef H_LOAD_SCANNER
#define H_LOAD_SCANNER

//include
#include <atomic_bool.hpp>
#include <boost/filesystem.hpp>
#include <boost/utility.hpp>
#include <global_thread_pool.hpp>
#include <logger.hpp>

class load_scanner : private boost::noncopyable
{
public:
	load_scanner();
	~load_scanner();

private:
	//when true scan() is stopped
	atomic_bool stop_flag;

	/*
	scan:
		Scans load directory.
	*/
	void scan();
};
#endif
