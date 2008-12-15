#include "request_generator.h"

request_generator::request_generator(
):
	current(0),
	initialized(false)
{

}

void request_generator::check_timeouts()
{
	//do re_request on requests that are older than timeout seconds
	std::map<boost::uint64_t, request_info>::iterator iter_cur, iter_end;
	iter_cur = unfulfilled_request.begin();
	iter_end = unfulfilled_request.end();
	while(iter_cur != iter_end){
		if(iter_cur->second.time > time(NULL) + timeout * iter_cur->second.pipeline_size){
			re_request.insert(iter_cur->first);
		}
		++iter_cur;
	}
}

bool request_generator::complete()
{
	boost::mutex::scoped_lock lock(Mutex);
	assert(initialized);
	return current == end && unfulfilled_request.empty() && re_request.empty() && re_requested.empty();
}

void request_generator::force_rerequest(const boost::uint64_t & number)
{
	boost::mutex::scoped_lock lock(Mutex);
	assert(initialized);
	re_request.insert(number);
}

void request_generator::fulfil(const boost::uint64_t & fulfilled_request)
{
	boost::mutex::scoped_lock lock(Mutex);
	assert(initialized);
	unfulfilled_request.erase(fulfilled_request);
	re_request.erase(fulfilled_request);
	re_requested.erase(fulfilled_request);
}

boost::uint64_t request_generator::highest_requested()
{
	boost::mutex::scoped_lock lock(Mutex);
	return current;
}

void request_generator::init(const boost::uint64_t & begin_in, const boost::uint64_t & end_in, const int & timeout_in)
{
	//assert that there is at least 1 request to be made
	assert(end_in - begin_in > 0);

	initialized = true;
	unfulfilled_request.clear();
	re_request.clear();
	re_requested.clear();
	current = begin_in;
	begin = begin_in;
	end = end_in;
	timeout = timeout_in;
}

bool request_generator::request(std::deque<boost::uint64_t> & prev_request)
{
	boost::mutex::scoped_lock lock(Mutex);
	assert(initialized);
	check_timeouts();
	if(re_request.empty()){
		//no re_requests to be made
		if(current == end){
			return false;
		}else{
			//proceed normally, adding the next sequential request
			prev_request.push_back(current);
			unfulfilled_request.insert(std::make_pair(current, request_info(time(NULL), prev_request.size())));
			++current;
			return true;
		}
	}else{
		/*
		A re_request needs to be done. Only do re_requests from servers that haven't
		already had a block re_requested from them.
		*/
		std::set<boost::uint64_t>::iterator re_requested_iter_cur, re_requested_iter_end;
		std::deque<boost::uint64_t>::iterator prev_iter_cur, prev_iter_end;
		re_requested_iter_cur = re_requested.begin();
		re_requested_iter_end = re_requested.end();
		while(re_requested_iter_cur != re_requested_iter_end){
			prev_iter_cur = prev_request.begin();
			prev_iter_end = prev_request.end();
			while(prev_iter_cur != prev_iter_end){
				if(*re_requested_iter_cur == *prev_iter_cur){
					/*
					A re-request was already made from this server. It won't be allowed
					to handle another re-request until it handles the one it already has.
					*/
					return false;
				}
				++prev_iter_cur;
			}
			++re_requested_iter_cur;
		}

		prev_request.push_back(*(re_request.begin()));
		unfulfilled_request.insert(std::make_pair(*(re_request.begin()), request_info(time(NULL), prev_request.size())));
		re_requested.insert(*(re_request.begin()));
		re_request.erase(re_request.begin());
		logger::debug(LOGGER_P1,"re_requesting ",*(prev_request.begin()));
		return true;
	}
}

void request_generator::set_timeout(const unsigned int & timeout_in)
{
	timeout = timeout_in;
}
