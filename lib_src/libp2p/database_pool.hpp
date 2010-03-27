#ifndef H_DATABASE_POOL
#define H_DATABASE_POOL

//custom
#include "path.hpp"
#include "settings.hpp"

//include
#include <boost/shared_ptr.hpp>
#include <db.hpp>
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
	The proxy automatically returns the connection to the pool when the last
	reference to it goes out of scope.
	*/
	class proxy
	{
	public:
		proxy();

		/*
		Exposing the shared_ptr with this operator makes it impossible for the
		shared_ptr to be copied. This is what we want since it reduces errors.
		*/
		boost::shared_ptr<connection> & operator -> ();

	private:
		boost::shared_ptr<connection> Connection;
		boost::shared_ptr<proxy> This;

		//when new_connection = false this deleter returns the connection to pool
		void deleter(boost::shared_ptr<connection> Connection);
	};

	/*
	get:
		Conveniance function for when a single database call needs to be done. The
		advantage of using this is that the connection is returned after the
		expression is evaluated.
	*/
	static proxy get();

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
