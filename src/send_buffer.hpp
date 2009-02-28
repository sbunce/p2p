/*
When two hosts are downloading files from eachother concurrently there is a
starvation problem where block requests get stuck behind blocks in the send
buffer.

To correct this, block requests are prioritized over blocks. Also slot requests
are prioritized to get files downloading as soon as possible.
*/
//NOT-THREADSAFE
#ifndef H_SEND_BUFFER
#define H_SEND_BUFFER

//boost
#include <boost/shared_ptr.hpp>

//custom
#include "global.hpp"

//include
#include <logger.hpp>

//std
#include <deque>
#include <string>

class send_buffer
{
public:
	send_buffer();

	/*
	add:
		Adds a message to be sent. This should be ONE message only. This function
		returns true if message was added, false if overpipelined. If returns
		false then server should be blacklisted.
	empty:
		Returns true if all buffers empty.
	get_send_buff:
		Clears destination and fills it with up to max_bytes of data to be sent.
		Returns true if destination is not empty upon completion.
	post_send:
		Erases sent data from the send_buffer.
	*/
	bool add(const std::string & buff);
	bool empty();
	bool get_send_buff(const int & max_bytes, std::string & destination);
	void post_send(const int & n_bytes);

private:
	/*
	Partially sent messages go in here. Messages are committed to being sent when
	part of them was sent.

	It is possible that a command will be added to this, and then later moved
	back to a uncommitted deque when it is not actually sent.
	*/
	std::deque<boost::shared_ptr<std::string> > buffer_committed;

	/*
	The buffer_response_uncommitted container holds responses to requests of the
	remote host.

	The buffer_request_uncommitted container holds requests we are making to the
	remote host. Requests in the request container are prioritized above
	responses in the response container.
	*/
	std::deque<boost::shared_ptr<std::string> > buffer_response_uncommitted;
	std::deque<boost::shared_ptr<std::string> > buffer_request_uncommitted;
};
#endif
