#ifndef H_DATABASE_POOL
#define H_DATABASE_POOL

//custom
#include "path.hpp"
#include "settings.hpp"

//include
#include <boost/shared_ptr.hpp>
#include <database_connection.hpp>
#include <singleton.hpp>

//standard
#include <stack>

namespace database{
class pool : public singleton_base<pool>
{
	friend class singleton_base<pool>;
public:
	virtual ~pool()
	{
		LOGGER << "pool destructed";
	}

	/*
	Manually get()'ing and put()'ing database connections is very error prone
	because it'd be easy for someone to get() and accidently forget to put() the
	connection back when finished. The proxy automatically returns the connection
	to the pool when done which eliminates the problem.

	Note: The proxy is meant to be used at the function scope. Do NOT declare one
		in a class that will be around for a long time.
	*/
	class proxy
	{
	public:
		proxy():
			Connection(singleton().get()),
			This(this, boost::bind(&proxy::deleter, this))
		{}

		boost::shared_ptr<connection> & operator -> (){ return Connection; }
	private:
		//connection that proxy wraps
		boost::shared_ptr<connection> Connection;

		/*
		Used to trigger deleter which passes Connection back to pool. Basically
		"This" is used to reference count for the proxy.
		*/
		boost::shared_ptr<proxy> This;

		void deleter()
		{
			singleton().put(Connection);
		}
	};

	/*
	This should be used when a single database call needs to be done. The
	advantage of this is that the connection will be returned after the
	expression rather than having to wait for the block to end. This is important
	because it returns the database connection to the pool ASAP.
	example:
		database::pool::get_proxy()->query("SELECT foo FROM bar");
	*/
	static proxy get_proxy()
	{
		return proxy();
	}

private:
	pool()
	{
		for(int x=0; x<settings::DATABASE_POOL_SIZE; ++x){
			Pool.push(boost::shared_ptr<connection>(new connection(path::database())));
		}
	}

	/*
	The Pool container holds available database connections. When there are no
	available connections threads will wait on the Pool_cond. The Pool_mutex
	locks access to the Pool.
	*/
	boost::mutex Pool_mutex;
	boost::condition_variable_any Pool_cond;
	std::stack<boost::shared_ptr<connection> > Pool;

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

	//the connection gotten with get() is returned with this
	void put(boost::shared_ptr<connection> & Connection)
	{
		boost::mutex::scoped_lock lock(Pool_mutex);
		Pool.push(Connection);
		Pool_cond.notify_one();
	}
};
}//end namespace database
#endif
