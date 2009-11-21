#ifndef H_EXCHANGE
#define H_EXCHANGE

//custom
#include "database.hpp"
#include "protocol.hpp"

//include
#include <boost/optional.hpp>
#include <network/network.hpp>

//standard
#include <list>
#include <vector>

class exchange
{
public:
	//message to send over the network
	class message
	{
	public:
		/*
		Data put in send_buf when sending a message. When we recieve a response to
		a request the request will be in send_buf, and the response in recv_buf.
		*/
		network::buffer recv_buf;
		network::buffer send_buf;

		/*
		If the message to send expects a response the possible responses are
		stored here.
		std::pair<command, size of response>
		*/
		std::vector<std::pair<unsigned char, unsigned> > expected_response;
	};

	/*
	The HAVE_HASH_TREE_BLOCK and HAVE_FILE_BLOCK SLOT_ID messages have VLIs in
	them but they are not in direct response to a command. To know the size of
	the VLI we memorize it when we see a SLOT_ID message.
	*/
//DEBUG, require slot_manager to register VLI_sizes
	class VLI_size_element
	{
	public:
		//size of VLIs (bytes)
		unsigned hash_tree_VLI_size;
		unsigned file_VLI_size;
	};
	std::map<unsigned char, VLI_size_element> VLI_size;

	/*
	recv:
		Returns shared_ptr to received message. Returns empty shared_ptr if a
		complete message hasn't been received or if the remote host was
		blacklisted.
	schedule_send:
		Add message to be sent.
	send:
		Returns message to send (or empty buffer if nothing to send).
	*/
	boost::shared_ptr<message> recv(network::connection_info & CI);
	void schedule_send(boost::shared_ptr<message> & M);
	network::buffer send();

private:
	//pending messages to send
	std::list<boost::shared_ptr<message> > Send;

	/*
	When a message is sent that expects a response it is pushed on to the back of
	this. When a response arrives it is to the message on the front.
	*/
	std::list<boost::shared_ptr<message> > Sent;
};
#endif
