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
	~pool(){}

	/*
	The proxy automatically returns the connection to the pool when the last copy
	of the proxy is destroyed. The proxy object should be kept for as little time
	as possible.
	*/
	class proxy
	{
	public:
		proxy():
			Connection(singleton().get()),
			This(this, boost::bind(&proxy::deleter, this, Connection))
		{}
		boost::shared_ptr<connection> & operator -> (){ return Connection; }
	private:
		boost::shared_ptr<connection> Connection;
		boost::shared_ptr<proxy> This;
		void deleter(boost::shared_ptr<connection> Connection)
		{
			singleton().put(Connection);
		}
	};

	/*
	get_proxy:
		This should be used when a single database call needs to be done. The
		advantage of this is that the connection will be returned after the
		expression rather than having to wait for the block to end. This is
		important because it returns the database connection to the pool ASAP.
		Example:
			database::pool::get_proxy()->query("SELECT foo FROM bar");
	unit_test_override:
		Removes the connections from the Pool and replaces them with connections
		to a database at a specified path. This should be called at the top of a
		unit test before any database use.
	*/
	static proxy get_proxy();
	void unit_test_override(const std::string & DB_path);

private:
	pool();

	/*
	The Pool container holds available database connections. When there are no
	available connections threads will wait on the Pool_cond. The Pool_mutex
	locks access to the Pool.
	*/
	boost::mutex Pool_mutex;
	boost::condition_variable_any Pool_cond;
	std::stack<boost::shared_ptr<connection> > Pool;

	/*
	get:
		Blocks until connection available to return.
	put:
		Function to return a connection to the pool.
	*/
	boost::shared_ptr<connection> get();
	void put(boost::shared_ptr<connection> & Connection);
};
}//end namespace database
#endif
