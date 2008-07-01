#ifndef H_REQUEST_GEN
#define H_REQUEST_GEN

//boost
#include <boost/thread/mutex.hpp>

//std
#include <ctime>
#include <deque>
#include <iostream>
#include <map>
#include <set>

//custom
#include "global.h"

class request_gen
{
public:
	request_gen();

	/*
	complete          - returns true if there are no more requests to be made
	force_re_request  - makes a request immediately be re-requested
	fulfil            - must be called whenever a request is fulfilled
	highest_requested - returns the highest request given
	init              - must be called before calling any other function
	request           - pushes a needed request number on the back of prev_request
	                    returns true if request pushed on to back of prev_request
	*/
	bool complete();
	void force_re_request(const uint64_t & number);
	void fulfil(const uint64_t & fulfilled_request);
	uint64_t highest_requested();
	void init(const uint64_t & min_request_in, const uint64_t & max_request_in, const int & timeout_in);
	bool request(std::deque<uint64_t> & prev_request);

private:
	/*
	This mutex is needed for the download_file which will request blocks as it is
	hash checking.

	This mutex should be used on all public functions except init().
	*/
	boost::mutex Mutex;

	//if false and a public member function called program will terminate
	bool initialized;

	/*
	The latest request returned by new_request. This will not be changed when
	new_request returns a re_request. This can only go forward one at a time.
	*/
	uint64_t latest_request;

	//min and max request number
	uint64_t min_request;
	uint64_t max_request;

	//max age of a request (in seconds) before a re_request is made
	int timeout;

	/*
	Requests that have been made associated with the time they were made. This is
	used to keep track of timeouts. If a element in this container times out it is
	re_requested.
	*/
	std::map<uint64_t, time_t> unfulfilled_request;

	/*
	Whenever a element in unfulfilled_request times out it's request number is
	put in the re_request container so that request() can issue another request
	(re-request) for the number.
	*/
	std::set<uint64_t> re_request;

	/*
	After request() serves a request from the re_request container it will erase
	the number from the re_request container and store it in the re_requested
	container.

	This container is necessary to be able to check whether or not a server has
	already had a block re-requested from it. This is needed because it you don't
	want to re-request the block from the server that's late sending it. Also,
	this evenly distributes the re-requests over servers to give the best possible
	chance of getting the requested block in a timely manner.
	*/
	std::set<uint64_t> re_requested;

	/*
	check_timeouts - if any requests have timed out add them to re_request
	*/
	void check_timeouts();
};
#endif
