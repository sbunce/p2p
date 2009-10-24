/*
The proactor gets jobs from a reactor and dispacthes them by calling the
appropriate call back. The proactor uses as many threads as there are CPUs.
*/
#ifndef H_NETWORK_PROACTOR
#define H_NETWORK_PROACTOR

//include
#include <boost/utility.hpp>
#include <logger.hpp>
#include <network/network.hpp>

//standard
#include <deque>
#include <string>

namespace network{

//predeclaration for PIMPL
class reactor;

class proactor
{
public:
	/*
	If duplicate_allowed is false then duplicate connections won't be allowed.
	If no port specified then no port will be listened on.
	*/
	proactor(
		boost::function<void (sock & Sock)> connect_call_back_in,
		boost::function<void (sock & Sock)> disconnect_call_back_in,
		boost::function<void (sock & Sock)> failed_connect_call_back_in,
		const bool duplicates_allowed,
		const std::string & port = ""
	);
	~proactor();

	/*
	This function starts the networking in the proactor and reactor threads.
	These threads are not started in the ctor because stuff may need to be done
	before the networking is brought up.
	*/
	void start();

	/*
	connect:
		Does asynchronous DNS resolution. Hands off DNS resolution to a thread
		inside the proactor which does DNS resolution, then calls
		reactor::connect.
	*/
	void connect(const std::string & host, const std::string & port);

	/*
	Functions that wrap reactor functions. Documentation for these can be found
	in reactor.hpp. The functions wrapped are named identically.
	*/
	unsigned connections();
	unsigned download_rate();
	unsigned incoming_connections();
	unsigned outgoing_connections();
	void max_connections(const unsigned max_incoming_connections_in,
		const unsigned max_outgoing_connections_in);
	unsigned max_download_rate();
	void max_download_rate(const unsigned rate);
	unsigned max_incoming_connections();
	unsigned max_outgoing_connections();
	unsigned max_upload_rate();
	void max_upload_rate(const unsigned rate);
	unsigned supported_connections();
	void trigger_call_back(const std::string & host, const std::string & port);
	unsigned upload_rate();

private:
	/*
	Reactor which the proactor uses. This object is public and can be used to get
	network information, set rate limits, etc.
	*/
	boost::shared_ptr<reactor> Reactor;

	//worker threads to handle call backs
	boost::thread_group Workers;

	//call backs used by dispatch thread
	boost::function<void (sock & Sock)> failed_connect_call_back;
	boost::function<void (sock & Sock)> connect_call_back;
	boost::function<void (sock & Sock)> disconnect_call_back;

	/*
	Resolve jobs queue'd by resolve().
	Note: job_mutex locks all access to job container.
	Note: job_cond is used to notify resolve thread when there is work.
	*/
	class resolve_job_element
	{
	public:
		enum type_enum {
			new_connection,
			force_call_back
		};

		resolve_job_element(){}
		resolve_job_element(
			const std::string & host_in,
			const std::string & port_in,
			const type_enum & type_in
		):
			host(host_in),
			port(port_in),
			type(type_in)
		{}

		std::string host;
		std::string port;
		type_enum type;
	};
	boost::mutex resolve_job_mutex;
	boost::condition_variable_any resolve_job_cond;
	std::deque<resolve_job_element> resolve_job;

	/*
	dispatch:
		Dispatches worker threads (does call backs) to handle jobs from the
		reactor.
	resolve:
		Does DNS resolution.
	*/
	void dispatch();
	void resolve();
};
}
#endif
