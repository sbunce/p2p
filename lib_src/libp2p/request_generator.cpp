#include "request_generator.hpp"

request_generator::request_generator():
	initialized(false)
{

}

void request_generator::init(const boost::uint64_t & begin_in, const boost::uint64_t & end_in,
	const int & timeout_in)
{
	assert(end_in - begin_in > 0);
	assert(timeout_in != 0);
	current = begin_in;
	begin = begin_in;
	end = end_in;
	timeout = timeout_in;
	initialized = true;
}

void request_generator::check_timeouts()
{
	//do re_request on requests that are older than timeout seconds
	for(std::map<boost::uint64_t, request_info>::iterator iter_cur = unfulfilled_request.begin(),
		iter_end = unfulfilled_request.end(); iter_cur != iter_end; ++iter_cur)
	{
		if(iter_cur->second.time > time(NULL) + timeout * iter_cur->second.pipeline_size){
			re_request.insert(iter_cur->first);
		}
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
	assert(initialized);
	return current;
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
		for(std::set<boost::uint64_t>::iterator re_requested_iter_cur = re_requested.begin(),
			re_requested_iter_end = re_requested.end(); re_requested_iter_cur != re_requested_iter_end;
			++re_requested_iter_cur)
		{
			for(std::deque<boost::uint64_t>::iterator prev_iter_cur = prev_request.begin(),
				prev_iter_end = prev_request.end(); prev_iter_cur != prev_iter_end; ++prev_iter_cur)
			{
				if(*re_requested_iter_cur == *prev_iter_cur){
					/*
					A re-request was already made from this server. It won't be allowed
					to handle another re-request until it handles the one it already has.
					*/
					return false;
				}
			}
		}

		prev_request.push_back(*(re_request.begin()));
		unfulfilled_request.insert(std::make_pair(*(re_request.begin()), request_info(time(NULL), prev_request.size())));
		re_requested.insert(*(re_request.begin()));
		re_request.erase(re_request.begin());
		return true;
	}
}

void request_generator::set_timeout(const unsigned & timeout_in)
{
	boost::mutex::scoped_lock lock(Mutex);
	assert(initialized);
	timeout = timeout_in;
}
