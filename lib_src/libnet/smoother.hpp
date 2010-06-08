#ifndef H_NET_SMOOTHER
#define H_NET_SMOOTHER

namespace network{
/*
Inserts sleeps in to a network loop to smooth out sends/recvs. When we have
rate limiting we 
*/
class smoother
{
public:
	smoother();

	/* Called Every Iteration
	do_sleep:
		May or may not do sleep.
	*/
	void do_sleep();

	/*
	allow_recv:
		Returns true if a recv is allowed.
	allow_send:
		Returns true if a send is allowed.
	recv_limit_reached:
		Must be called when recv limit reached.
	send_limit_reached:
		Must be called when send limit reached.
	*/
	bool allow_recv();
	bool allow_send();
	void recv_limit_reached();
	void send_limit_reached();

private:
	//used to do actions once per second
	std::time_t time;
};
}//end namespace network
#endif
