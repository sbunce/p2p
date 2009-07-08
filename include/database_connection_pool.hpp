/*
This is a very simple singleton container. It holds database::connection's to
do connection pooling.
*/
#ifndef H_DATABASE_CONNECTION_POOL
#define H_DATABASE_CONNECTION_POOL

//include
#include <boost/shared_ptr.hpp>
#include <database_connection.hpp>
#include <singleton.hpp>

//standard
#include <stack>

namespace database{

class connection_pool : public singleton_base<connection_pool>
{
	friend class singleton_base<connection_pool>;
public:

	//blocks until there is an available connection
	boost::shared_ptr<connection> get()
	{
		boost::mutex::scoped_lock lock(Pool_mutex);
		while(Pool.empty()){
			Pool_cond.wait(Pool_mutex);
		}
		boost::shared_ptr<connection> Connection = Pool.top();
		Pool.pop();
		return Connection;
	}

	/*
	The connection gotten with get() should be returned with this.
	Note: As a safety feature the shared_ptr passed to this function will be set
		to NULL so that an assert will be triggered if it's used. This stops
		someone from accidently introducing a thread-unsafe condition where they
		put() the connection back, but then continue to use it.
	*/
	void put(boost::shared_ptr<connection> & Connection)
	{
		boost::mutex::scoped_lock lock(Pool_mutex);
		Pool.push(Connection);
		Pool_cond.notify_one();
		Connection = boost::shared_ptr<connection>();
	}

private:
	connection_pool(){}

	boost::mutex Pool_mutex;
	boost::condition_variable_any Pool_cond;
	std::stack<boost::shared_ptr<connection> > Pool;
};
}//end namespace database
#endif
