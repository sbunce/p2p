#include "request_gen.h"

request_gen::request_gen(uint64_t min_request_in, uint64_t max_request_in, int timeout_in)
:
latest_request(min_request_in),
min_request(min_request_in),
max_request(max_request_in),
timeout(timeout_in)
{

}

void request_gen::check_re_requests()
{
	//do re_requests on requests that are older than max_age.
	std::map<uint64_t, time_t>::iterator iter_cur, iter_end;
	iter_cur = requests.begin();
	iter_end = requests.end();
	while(iter_cur != iter_end){
		if(time(0) - iter_cur->second > timeout){
			re_requests.insert(std::make_pair(iter_cur->first, time(0)));
		}
		++iter_cur;
	}
}

bool request_gen::complete()
{
	if(latest_request == max_request && requests.empty() && re_requests.empty()){
		return true;
	}else{
		return false;
	}
}

void request_gen::fulfilled(uint64_t fulfilled_request)
{
	requests.erase(fulfilled_request);
	re_requests.erase(fulfilled_request);
}

uint64_t request_gen::highest_request()
{
	return latest_request;
}

bool request_gen::new_request_check_existing(std::deque<uint64_t> & prev_requests)
{
	//do not do re_requests from servers that are having a block re_requested
	std::map<uint64_t, time_t>::iterator re_requests_iter_cur, re_requests_iter_end;
	std::deque<uint64_t>::iterator prev_iter_cur, prev_iter_end;

	re_requests_iter_cur = re_requests.begin();
	re_requests_iter_end = re_requests.end();
	while(re_requests_iter_cur != re_requests_iter_end){
		prev_iter_cur = prev_requests.begin();
		prev_iter_end = prev_requests.end();
		while(prev_iter_cur != prev_iter_end){
			if(re_requests_iter_cur->first == *prev_iter_cur){
				return false;
			}
			++prev_iter_cur;
		}
		++re_requests_iter_cur;
	}

	prev_requests.push_back(re_requests.begin()->first);

	logger::debug(LOGGER_P1,"re_requesting ",re_requests.begin()->first);
	re_requests.erase(re_requests.begin()->first);
	return true;
}

bool request_gen::new_request(std::deque<uint64_t> & prev_requests)
{
	check_re_requests();
	if(re_requests.empty()){
		if(latest_request == max_request){
			//max request reached
			if(prev_requests.empty()){
				return false;
			}else{
				if(prev_requests.back() != max_request){
					//max_request hasn't yet been requested
					prev_requests.push_back(max_request);
					requests.insert(std::make_pair(max_request, time(0)));
					return true;
				}else{
					//max request already requested
					return false;
				}
			}
		}else{
			prev_requests.push_back(latest_request);
			requests.insert(std::make_pair(latest_request, time(0)));
			++latest_request;
			return true;
		}
	}else{
		return new_request_check_existing(prev_requests);
	}
}
