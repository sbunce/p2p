#include "request_gen.h"

request_gen::request_gen(
):
	latest_request(0),
	initialized(false)
{

}

void request_gen::check_timeouts()
{
	//do re_request on requests that are older than max_age.
	std::map<uint64_t, time_t>::iterator iter_cur, iter_end;
	iter_cur = unfulfilled_request.begin();
	iter_end = unfulfilled_request.end();
	while(iter_cur != iter_end){
		if(time(0) - iter_cur->second > timeout){
			re_request.insert(std::make_pair(iter_cur->first, time(0)));
		}
		++iter_cur;
	}
}

bool request_gen::complete()
{
	assert(initialized);
	return (latest_request == max_request && unfulfilled_request.empty() && re_request.empty());
}

void request_gen::force_re_request(const uint64_t & number)
{
	assert(initialized);
	//alter time to force re_request
	std::map<uint64_t, time_t>::iterator iter = unfulfilled_request.find(number);
	if(iter != unfulfilled_request.end()){
		iter->second = 0; //set time requested to zero, which will trigger re_request
	}else{
		unfulfilled_request.insert(std::make_pair(number, 0));
	}
}

void request_gen::fulfil(uint64_t fulfilled_request)
{
	assert(initialized);
	unfulfilled_request.erase(fulfilled_request);
	re_request.erase(fulfilled_request);
}

uint64_t request_gen::highest_requested()
{
	assert(initialized);
	return latest_request;
}

bool request_gen::check_re_request(std::deque<uint64_t> & prev_request)
{
	//do not do re_request from servers that are having a block re_requested
	std::map<uint64_t, time_t>::iterator re_request_iter_cur, re_request_iter_end;
	std::deque<uint64_t>::iterator prev_iter_cur, prev_iter_end;
	re_request_iter_cur = re_request.begin();
	re_request_iter_end = re_request.end();
	while(re_request_iter_cur != re_request_iter_end){
		prev_iter_cur = prev_request.begin();
		prev_iter_end = prev_request.end();
		while(prev_iter_cur != prev_iter_end){
			if(re_request_iter_cur->first == *prev_iter_cur){
				return false;
			}
			++prev_iter_cur;
		}
		++re_request_iter_cur;
	}

	prev_request.push_back(re_request.begin()->first);
	logger::debug(LOGGER_P1,"re_requesting ",re_request.begin()->first);
	re_request.erase(re_request.begin()->first);
	return true;
}

void request_gen::init(const uint64_t & min_request_in, const uint64_t & max_request_in, const int & timeout_in)
{
	initialized = true;
	unfulfilled_request.clear();
	re_request.clear();
	latest_request = min_request_in;
	min_request = min_request_in;
	max_request = max_request_in;
	timeout = timeout_in;
}

bool request_gen::request(std::deque<uint64_t> & prev_request)
{
	assert(initialized);
	check_timeouts();
	if(re_request.empty()){
		if(latest_request == max_request){
			//max request reached
			if(prev_request.empty()){
				return false;
			}else{
				if(prev_request.back() != max_request){
					//max_request hasn't yet been requested
					prev_request.push_back(max_request);
					unfulfilled_request.insert(std::make_pair(max_request, time(0)));
					return true;
				}else{
					//max request already requested
					return false;
				}
			}
		}else{
			prev_request.push_back(latest_request);
			unfulfilled_request.insert(std::make_pair(latest_request, time(0)));
			++latest_request;
			return true;
		}
	}else{
		return check_re_request(prev_request);
	}
}
