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
	max_connections_supported:
		Maximum number of connections the reactor supports.
	start:
		Start the reactor.
	stop:
		Stop the reactor.
	*/
	virtual unsigned max_connections_supported() = 0;
	virtual void start() = 0;
	virtual void stop() = 0;

	/*
	connect:
		Asynchronously connect to a host. This function returns immediately.
	connections:
		Returns current total connections, both incoming and outgoing.
	current_download_rate:
		Returns current download rate (bytes/second).
	current_upload_rate:
		Returns current upload rate (bytes/second).
	get_job:
		Block until a sock is modified. The sock will have flags set that will
		tell the proactor what to do with it.
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
	put_job:
		The socks gotten from get_job are returned with this function.
	*/
	void connect(boost::shared_ptr<sock> & S);
	unsigned connections();
	unsigned current_download_rate();
	unsigned current_upload_rate();
	boost::shared_ptr<sock> get_job();
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
	void put_job(boost::shared_ptr<sock> & S);

protected:
	//controls maximum upload/download rate
	rate_limit Rate_Limit;

	/* Pure Virtuals
	trigger_selfpipe:
		See selfpipe documentation in reactor_select.
	*/
	virtual void trigger_selfpipe() = 0;

	/*
	add_job:
		The derived reactor schedules a job by passing a sock to this function.
		The job is picked up by the proactor when it calls get_job().
		Note: The reactor does not monitor the socket for activity until it is
			returned.
	get_job_connect:
		The derived reactor gets a connect job scheduled with connect(). An empty
		shared_ptr is returned if there are no connect jobs.
	get_finished_job:
		Get socks that were returned by the proactor with put_job(). An empty
		shared_ptr is returned if there are no finished jobs.
	incoming_decrement:
		Decrements the incoming connection count.
		Precondition: incoming_connections != 0.
	incoming_increment:
		Increments the incoming connection count.
		Precondition: A sock must exist with a socket_FD != -1. This is because
			incoming_decrement() will be called for a sock with a socket_FD != -1.
			We want there to be a decrement for every increment.
		Precondition: incoming_connections < _max_incoming_connections.
	incoming_limit_reached:
		Returns true if incoming connected limit has been reached.
	outgoing_decrement:
		Decrements the outgoing connection count.
		Precondition: outgoing_connections != 0.
	outgoing_increment:
		Increments the outgoing connection count.
		Precondition: A sock must exist with a socket_FD != -1. This is because
			outgoing_decrement() will be called for a sock with a socket_FD != -1. We
			want there to be a decrement for every increment.
		Precondition: outgoing_connections < _max_outgoing_connections.
	outgoing_limit_reached:
		Return true if outgoing connection limit reached.
	*/
	void add_job(boost::shared_ptr<sock> & S);
	boost::shared_ptr<sock> get_job_connect();
	boost::shared_ptr<sock> get_finished_job();
	void incoming_decrement();
	void incoming_increment();
	bool incoming_limit_reached();
	void outgoing_decrement();
	void outgoing_increment();
	bool outgoing_limit_reached();

private:
	/*
	The connect() function puts sock's in the connect_job container to hand them
	off to the main_loop_thread which will do an asynchronous connection.
	*/
	boost::mutex job_connect_mutex;
	std::deque<boost::shared_ptr<sock> > job_connect;

	/*
	When a sock needs to be passed back to the client (because it has a flag set)
	it is put in the job container and job_cond is notified.
	*/
	boost::mutex job_mutex;
	boost::condition_variable_any job_cond;
	std::deque<boost::shared_ptr<sock> > job;

	/*
	This finished_job container is used by put() to pass a sock back to the
	main_loop_thread which will start monitoring it for activity.
	*/
	boost::mutex finished_job_mutex;
	std::deque<boost::shared_ptr<sock> > finished_job;

	/* Connection counts and limits.
	Note: All these must be locked with connections_mutex.
	_incoming_connections:
		Number of connections remote hosts established with us.
	_max_incoming_connections:
		Maximum possible incoming connections.
	_outgoing_connections:
		Number of connections we established with remote hosts.
	_max_outgoing_connection:
		Maximum possible outgoing connections.
	*/
	boost::mutex connections_mutex;
	unsigned _incoming_connections;
	unsigned _max_incoming_connections;
	unsigned _outgoing_connections;
	unsigned _max_outgoing_connections;
};
}//end namespace network
#endif
