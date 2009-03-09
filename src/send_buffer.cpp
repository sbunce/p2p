#include "send_buffer.hpp"

send_buffer::send_buffer()
{

}

/*
Think about having functions add_request, add_response instead of figuring it
out here.
*/
bool send_buffer::add(const std::string & buff)
{
	int command_type = protocol::command_type(buff[0]);
	if(command_type == protocol::COMMAND_REQUEST){
		buffer_request_uncommitted.push_back(boost::shared_ptr<std::string>(new std::string(buff)));
	}else if(command_type == protocol::COMMAND_RESPONSE){
		buffer_response_uncommitted.push_back(boost::shared_ptr<std::string>(new std::string(buff)));
	}else{
		LOGGER << "remote host sent invalid command";
		return false;
	}

	#ifndef NDEBUG
	if(buffer_request_uncommitted.size() > protocol::PIPELINE_SIZE){
		LOGGER << "programmer error, we over pipelined";
		exit(1);
	}
	#endif

	if(buffer_response_uncommitted.size() > protocol::PIPELINE_SIZE){
		LOGGER << "remote host over pipelined";
		return false;
	}else{
		return true;
	}
}

void add_committed(const std::string & buff)
{
/*
This won't work because the post send function will try to add it back to an
uncommitted buffer.
*/
	//buffer_committed.push_back(boost::shared_ptr<std::string>(new std::string(buff));
}

bool send_buffer::empty()
{
	return buffer_committed.empty() && buffer_response_uncommitted.empty() && buffer_request_uncommitted.empty();
}

bool send_buffer::get_send_buff(const int & max_bytes, std::string & destination)
{
	#ifndef NDEBUG
	if(buffer_committed.size() > 1){
		LOGGER << "programmer error";
		exit(1);
	}
	#endif
	destination.clear();
	int bytes_remaining = max_bytes;
	while(bytes_remaining){
		if(!buffer_request_uncommitted.empty() && buffer_request_uncommitted.front()->size() <= bytes_remaining){
			buffer_committed.push_back(buffer_request_uncommitted.front());
			buffer_request_uncommitted.pop_front();
			destination += *buffer_committed.back();
			bytes_remaining -= buffer_committed.back()->size();
		}else if(!buffer_response_uncommitted.empty() && buffer_response_uncommitted.front()->size() <= bytes_remaining){
			buffer_committed.push_back(buffer_response_uncommitted.front());
			buffer_response_uncommitted.pop_front();
			destination += *buffer_committed.back();
			bytes_remaining -= buffer_committed.back()->size();
		}else{
			break;
		}
	}
	return !destination.empty();
}

void send_buffer::post_send(const int & n_bytes)
{
	//remove bytes sent
	int bytes_remaining = n_bytes;
	while(bytes_remaining && !buffer_committed.empty()){
		if(bytes_remaining >= buffer_committed.front()->size()){
			bytes_remaining -= buffer_committed.front()->size();
			buffer_committed.pop_front();
		}else{
			//only sent part of a message
			buffer_committed.front()->erase(0, bytes_remaining);
			bytes_remaining = 0;
		}
	}

	#ifndef NDEBUG
	if(bytes_remaining && buffer_committed.empty()){
		LOGGER << "programmer error, sending more bytes than we have?";
		exit(1);
	}
	#endif

	//push unsent back on to uncommitted
	if(!buffer_committed.empty()){
		//front might be partially sent and will not be uncommitted
		boost::shared_ptr<std::string> front = buffer_committed.front();
		buffer_committed.pop_front();

		int command_type;
		std::deque<boost::shared_ptr<std::string> >::reverse_iterator iter_cur, iter_end;
		iter_cur = buffer_committed.rbegin();
		iter_end = buffer_committed.rend();
		while(iter_cur != iter_end){
			command_type = protocol::command_type((**iter_cur)[0]);
			if(command_type == protocol::COMMAND_REQUEST){
				buffer_request_uncommitted.push_back(*iter_cur);
			}else if(command_type == protocol::COMMAND_RESPONSE){
				buffer_response_uncommitted.push_back(*iter_cur);
			}
			#ifndef NDEBUG
			else{
				//if there is an invalid command it should have been picked up by add
				LOGGER << "programmer error";
			}
			#endif
			++iter_cur;
		}

		buffer_committed.clear();
		buffer_committed.push_back(front);
	}
}
