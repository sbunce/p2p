#ifndef H_CONNECTION
#define H_CONNECTION

//custom
#include "encryption.hpp"
#include "message.hpp"
#include "share.hpp"
#include "slot_manager.hpp"

//include
#include <boost/utility.hpp>
#include <network/network.hpp>

class connection : private boost::noncopyable
{
public:
	connection(
		network::proactor & Proactor_in,
		network::connection_info & CI
	);

	const std::string IP;
	const int connection_ID;

private:

	class key_exchange_p_rA : public message
	{
	public:
		virtual bool expects(network::connection_info & CI)
		{
			//p rA doesn't begin with any command
			return true;
		}
		virtual bool parse(network::connection_info & CI)
		{
			if(CI.recv_buf.size() >= protocol::DH_KEY_SIZE * 2){
				buf.append(CI.recv_buf.data(), protocol::DH_KEY_SIZE * 2);
				CI.recv_buf.erase(0, protocol::DH_KEY_SIZE * 2);
				return true;
			}
			return false;
		}
		virtual message_type type(){ return message::key_exchange_p_rA; }
	};

	class key_exchange_rB : public message
	{
	public:
		virtual bool expects(network::connection_info & CI)
		{
			//rB doesn't begin with any command
			return true;
		}
		virtual bool parse(network::connection_info & CI)
		{
			if(CI.recv_buf.size() >= protocol::DH_KEY_SIZE){
				buf.append(CI.recv_buf.data(), protocol::DH_KEY_SIZE);
				CI.recv_buf.erase(0, protocol::DH_KEY_SIZE);
				return true;
			}
			return false;
		}
		virtual message_type type(){ return message::key_exchange_rB; }
	};

	class initial : public message
	{
	public:
		virtual bool expects(network::connection_info & CI)
		{
			//initial doesn't begin with any command
			return true;
		}
		virtual bool parse(network::connection_info & CI)
		{
			if(CI.recv_buf.size() >= SHA1::bin_size){
				buf.append(CI.recv_buf.data(), SHA1::bin_size);
				CI.recv_buf.erase(0, SHA1::bin_size);
				return true;
			}
			return false;
		}
		virtual message_type type(){ return message::initial; }
	};

	//locks all entry points to the object
	boost::recursive_mutex Recursive_Mutex;

	/*
	When a message is sent that expects a response it is pushed on the back of
	Expected. The request member of the message_pair stores the request made
	(but only if it's later needed). The response member is used to parse the
	incoming response.
	*/
	std::list<message_pair> Expected;

	network::proactor & Proactor;
	encryption Encryption;
	slot_manager Slot_Manager;
	std::string peer_ID; //holds peer_ID when it's received
	int blacklist_state; //used to know if blacklist updated

	/*
	recv_call_back:
		Proactor calls back to this function whenever data is received.
	send_initial:
		Send initial messages. Called after key exchange completed.
	*/
	void recv_call_back(network::connection_info & CI);
	void send(boost::shared_ptr<message> M);
	void send_initial();
};
#endif
