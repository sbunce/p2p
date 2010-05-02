#ifndef H_NETWORK_ID_MANAGER
#define H_NETWORK_ID_MANAGER

//include
#include <boost/thread.hpp>
#include <logger.hpp>
#include <set>

namespace network{
/*
This class allocates connection_IDs. There are a few problems this class
addresses:
1. We don't want to let the socket_FDs leave the proactor because the
	proactor operates on sockets asynchronously and may disconnect/reconnect
	while a call back is taking place.
2. The connection_IDs would have the same problem as #1 if we reused
	connection_IDs. To avoid the problem we continually allocate
	connection_IDs one greater than the last allocated connection_ID.
3. It is possible that the connection_IDs will wrap around and reuse existing
	connection_IDs. We maintain a set of used connection_IDs so we can prevent
	this.
*/
class ID_manager : private boost::noncopyable
{
public:
	ID_manager();

	/*
	allocate:
		Allocate connection_ID. The deallocate function should be called when
		done with the connection_ID.
	deallocate:
		Deallocate connection_ID. Allows the connection_ID to be reused.
	*/
	int allocate();
	void deallocate(int connection_ID);

private:
	boost::mutex Mutex;      //locks all access to object
	int highest_allocated;   //last connection_ID allocated
	std::set<int> allocated; //set of all connection_IDs allocated
};
}//end namespace network
#endif
