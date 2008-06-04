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
	complete        - returns true if there are no more requests to be made
	fulfilled       - must be called whenever a request is fulfilled
	init            - initialize the request generator with first and last request numbers
	highest_request - returns latest_request
	new_request     - pushes a needed request number on the back of prev_request
	                  returns false if no request needed
	resize_buffer   - changes the maximum buffer size.
	*/
	bool complete();
	void fulfilled(uint64_t fulfilled_request);
	void init(uint64_t min_request_in, uint64_t max_request_in, int max_age_in);
	uint64_t highest_request();
	bool new_request(std::deque<uint64_t> & prev_request);

private:
	/*
	The latest request returned by new_request. This will not be changed when
	new_request returns a re_request. This can only go forward one at a time.
	*/
	uint64_t latest_request;

	//min and max request number
	uint64_t min_request;
	uint64_t max_request;

	//max age of a request (in seconds) before a re_request is made
	int max_age;

	std::map<uint64_t, time_t> requests;    //requests, associated with time request made
	std::map<uint64_t, time_t> re_requests; //re_requested blocks, associated with time re_request made

	/*
	check_re_requests          - fills re_requests
	new_request_check_existing - issues a rerequest if no rerequest currently in prev_requests
	                             returns false if rerequest found in prev_requests else true
	*/
	void check_re_requests();
	bool new_request_check_existing(std::deque<uint64_t> & prev_requests);
};
#endif
