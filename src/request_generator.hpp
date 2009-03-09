//THREADSAFE
#ifndef H_REQUEST_GENERATOR
#define H_REQUEST_GENERATOR

//boost
#include <boost/thread.hpp>

//custom
#include "settings.hpp"

//std
#include <ctime>
#include <deque>
#include <iostream>
#include <map>
#include <set>

class request_generator
{
public:
	request_generator(
		const boost::uint64_t & begin_in, //start of range to request
		const boost::uint64_t & end_in,   //end of range to request
		const int & timeout_in            //timeout before rerequest
	);

	/*
	complete          - returns true when requests are complete
	force_rerequest   - rerequest a number immediately
	fulfil            - must be called whenever a request is fulfilled (otherwise rerequest will happen upon timeout)
	highest_requested - returns the highest request yet made
	request           - push a request number on the back of pre_request
	                    returns true if request pushed, else false if no request needed
	set_timeout       - set rerequest timeout
	*/
	bool complete();
	void force_rerequest(const boost::uint64_t & number);
	void fulfil(const boost::uint64_t & fulfilled_request);
	boost::uint64_t highest_requested();
	bool request(std::deque<boost::uint64_t> & prev_request);
	void set_timeout(const unsigned & timeout_in);

private:
	//mutex for all public functions
	boost::mutex Mutex;

	/*
	This is the highest request yet made. This will not be changed when
	new_request returns a re_request. This can only go forward one at a time.
	*/
	boost::uint64_t current;

	//min and max request number
	boost::uint64_t begin;
	boost::uint64_t end;   //request generator unitialized if this is zero

	//max age of a request (in seconds) before a re_request is made
	unsigned timeout;

	/*
	Requests that have been made associated with the time they were made. This is
	used to keep track of timeouts. If a element in this container times out it is
	re_requested.
	*/
	class request_info
	{
	public:
		request_info(
			const time_t & time_in,
			const int & pipeline_size_in
		):
			time(time_in),
			pipeline_size(pipeline_size_in)
		{}
		time_t time;       //time which the request was made
		int pipeline_size; //how large pipeline was when request made
	};
	std::map<boost::uint64_t, request_info> unfulfilled_request;

	/*
	Whenever a element in unfulfilled_request times out it's request number is
	put in the re_request container so that request() can issue another request
	(re-request) for the number.
	*/
	std::set<boost::uint64_t> re_request;

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
	std::set<boost::uint64_t> re_requested;

	/*
	check_timeouts - if any requests have timed out add them to re_request
	*/
	void check_timeouts();
};
#endif
