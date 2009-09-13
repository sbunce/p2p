/*
The reactor contains the thread which does the networking. Reactors must be
accessed through this class. It is not possible to access the derived reactor
directly.

Note: It's a major PITA to use the reactor by itself. It is much easier to use
the reactor with the proactor.
*/
#ifndef H_NETWORK_REACTOR
#define H_NETWORK_REACTOR

//custom
#include "rate_limit.hpp"
#include "wrapper.hpp"

//include
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <network/network.hpp>

//standard
#include <set>

namespace network{
class reactor : private boost::noncopyable
{
public:
	/*
	Maximum size sent in one shot. The actual hardware MTU will likely be less.
	On the internet the MTU will probably be around 1500. This maximum size was
	chosen because some LANs go up to 2^14 with "jumbo frames".
	*/
	static const int MTU = 16384;

	reactor();
	virtual ~reactor();

	/* Pure Virtuals
	start:
		Start the reactor. Called by the ctor of the proactor.
	stop:
		Stop the reactor. Called by the dtor of the proactor.
	supported_connections:
		Maximum number of connections the reactor supports.
	*/
	virtual void start() = 0;
	virtual void stop() = 0;
	virtual unsigned supported_connections() = 0;

	/*
	call_back_get_job:
		Block until a sock is modified. The sock will have flags set that will
		tell the proactor what to do with it.
	call_back_return_job:
		The socks gotten from get_job are returned with this function.
	connections:
		Returns current total connections. Incoming, outgoing, and outgoing
		connections in progress of connecting.
	download_rate:
		Returns current download rate (bytes/second).
	incoming_connections:
		Returns current incoming connection count.
	max_connections:
		Sets maximum number of incoming/outgoing connections.
		Note: max_incoming_connections_in + max_outgoing_connections_in must not
			exceed max_connection_supported().
	max_download_rate:
		Get maximum download rate (bytes/second). Get maximum download rate
		(bytes/second).
	max_incoming_connections:
		Returns maximum allowed incoming connections.
	max_outgoing_connections:
		Returns maximum allowed outgoing connections.
	max_upload_rate:
		Get maximum allowed upload rate (bytes/second). Set maximum allowed upload
		rate (bytes/second).
	outgoing_connections:
		Returns current outgoing connection count.
	schedule_connect:
		Asynchronously connect to a host. This function returns immediately.
	trigger_call_back:
		Triggers a call back for a sock with a specified IP. If no sock with the
		specified host is found then no call back will be done. If a call back is
		done it will be a recv call back and network::sock::latest_recv will be
		equal to 0. A call back will not be done for a socket in progress of
		connecting.
	upload_rate:
		Returns current upload rate (bytes/second).
	*/
	unsigned connections();
	boost::shared_ptr<sock> call_back_get_job();
	void call_back_return_job(boost::shared_ptr<sock> & S);
	unsigned download_rate();
	unsigned incoming_connections();
	void max_connections(const unsigned max_incoming_connections_in,
		const unsigned max_outgoing_connections_in);
	unsigned max_download_rate();
	void max_download_rate(const unsigned rate);
	unsigned max_incoming_connections();
	unsigned max_outgoing_connections();
	unsigned max_upload_rate();
	void max_upload_rate(const unsigned rate);
	unsigned outgoing_connections();
	void schedule_connect(boost::shared_ptr<sock> & S);
	void trigger_call_back(const std::string & IP);
	unsigned upload_rate();

protected:
	//controls maximum upload/download rate
	rate_limit Rate_Limit;

	/* Pure Virtuals
	trigger_selfpipe:
		See selfpipe documentation in reactor_select.
	*/
	virtual void trigger_selfpipe() = 0;

	/*
	call_back_finished_job:
		Get a sock that was returned by the proactor with put_job(). An empty
		shared_ptr is returned if there are no finished jobs.
	call_back_schedule_job:
		The derived reactor schedules a job by passing a sock to this function.
		The job is picked up by the proactor when it calls get_job().
		Note: The reactor does not monitor the socket for activity until it is
			returned to the reactor.
	connect_job:
		The derived reactor gets a connect job scheduled with connect(). An empty
		shared_ptr is returned if there are no connect jobs.
	connection_add:
		This is called whenever there is an outgoing connection attempt or an
		incoming connection. This is used for connection limiting.
	connection_incoming_limit:
		Returns true if incoming connected limit has been reached.
	connection_outgoing_limit:
		Return true if outgoing connection limit reached.
	connection_remove:
		This is called whenever a connection attempt fails, or a connected socket
		disconnects. This is used for connection limiting.
		Precondition: add_connection() must have been called for sock.
	force_check:
		Return true if the specified sock needs to have a forced call back done.
	force_pending:
		Returns true if there exists at least one sock which need to have a call
		back forced. If there are then force_check will be called for every sock
		in the derived reactor to see if a call back needs to be forced.
	is_duplicate:
		Returns true if the connection is a duplicate.
	*/
	boost::shared_ptr<sock> call_back_finished_job();
	void call_back_schedule_job(boost::shared_ptr<sock> & S);
	boost::shared_ptr<sock> connect_job_get();
	void connection_add(boost::shared_ptr<sock> & S);
	bool connection_incoming_limit();
	bool connection_outgoing_limit();
	void connection_remove(boost::shared_ptr<sock> & S);
	bool force_check(boost::shared_ptr<sock> & S);
	bool force_pending();
	bool is_duplicate(boost::shared_ptr<sock> & S);

private:
	/*
	The connect() function puts sock's in the connect_job container to hand them
	off to the main_loop_thread which will do an asynchronous connection.
	*/
	boost::mutex connect_job_mutex;
	std::deque<boost::shared_ptr<sock> > connect_job;

	/*
	When a sock has something done to it (as indicated by the flag set). It is
	put in this container to be given to the proactor when it calls
	call_back_get_job().
	*/
	boost::mutex schedule_job_mutex;
	boost::condition_variable_any schedule_job_cond;
	std::deque<boost::shared_ptr<sock> > schedule_job;

	/*
	This finished_job container is used by put() to pass a sock back to the
	main_loop_thread which will start monitoring it for activity.
	*/
	boost::mutex finished_job_mutex;
	std::deque<boost::shared_ptr<sock> > finished_job;

	/* Connection Limits
	Note: All these must be locked with connections_mutex.
	max_incoming:
		Maximum possible incoming connections.
	max_outgoing:
		Maximum possible outgoing connections.
	incoming:
		All incoming socks. Accessing non-threadsafe members of the socks in this
		container is not thread safe. The reactor doesn't necessarily have the
		right to modify these socks at all times.
	outgoing:
		All outgoing socks. Accessing non-threadsafe members of the socks in this
		container is not thread safe. The reactor doesn't necessarily have the
		right to modify these socks at all times.
	force_call_back:
		Contains socks which the client has requested we force a call back on.
		Accessing non-threadsafe members of the socks in this container is not
		thread safe. The reactor doesn't necessarily have the
		right to modify these socks at all times.
	*/
	boost::mutex connections_mutex;
	unsigned max_incoming;
	unsigned max_outgoing;
	std::set<boost::shared_ptr<sock> > incoming;
	std::set<boost::shared_ptr<sock> > outgoing;
	std::set<boost::shared_ptr<sock> > force_call_back;
};
}//end namespace network
#endif
