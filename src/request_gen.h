#ifndef H_REQUEST_GEN
#define H_REQUEST_GEN

//std
#include <ctime>
#include <deque>
#include <iostream>
#include <map>

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
	void fulfil(uint64_t fulfilled_request);
	uint64_t highest_requested();
	void init(const uint64_t & min_request_in, const uint64_t & max_request_in, const int & timeout_in);
	bool request(std::deque<uint64_t> & prev_request);

private:
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

	std::map<uint64_t, time_t> unfulfilled_request; //requests, associated with time request made
	std::map<uint64_t, time_t> re_request;          //re_requested blocks, associated with time re_request made

	/*
	check_re_requests - if any requests have timed out add them to re_request
	check_re_request  - issues a rerequest if no rerequests currently in prev_requests
	                    returns false if rerequest found in prev_requests else true
	*/
	void check_timeouts();
	bool check_re_request(std::deque<uint64_t> & prev_requests);
};
#endif
