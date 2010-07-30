#ifndef H_DB_POOL
#define H_DB_POOL

//custom
#include "path.hpp"
#include "settings.hpp"

//include
#include <boost/shared_ptr.hpp>
#include <db.hpp>
#include <singleton.hpp>

//standard
#include <stack>

namespace db{
class pool : public singleton_base<pool>
{
	friend class singleton_base<pool>;
public:
	~pool(){}

	class proxy
	{
		friend class pool;
	public:
		/*
		Exposing the shared_ptr with this operator makes it impossible for the
		shared_ptr to be copied. This is what we want since it reduces errors.
		*/
		boost::shared_ptr<connection> & operator -> ();

	private:
		//only db::pool can instantiate proxy
		proxy();

		boost::shared_ptr<connection> Connection;
		boost::shared_ptr<proxy> This;

		//when new_connection = false this deleter returns the connection to pool
		void deleter(boost::shared_ptr<connection> Connection);
	};

	/*
	get:
		Get database object. This is the only way to get one.
	*/
	proxy get();

private:
	pool();

	/*
	The Pool container holds available database connections. When there are no
	available connections threads will wait on the Cond. The Mutex
	locks access to the Pool.
	*/
	boost::mutex Mutex;
	boost::condition_variable_any Cond;
	std::stack<boost::shared_ptr<connection> > Pool;

	/*
	pool_get:
		Blocks until connection can be returned.
	pool_put:
		Function to return connection to pool.
	*/
	boost::shared_ptr<connection> pool_get();
	void pool_put(boost::shared_ptr<connection> & Connection);
};
}//end namespace database
#endif
