#include "database_pool.hpp"

database::pool::pool()
{
	for(int x=0; x<settings::DATABASE_POOL_SIZE; ++x){
		Pool.push(boost::shared_ptr<connection>(new connection(path::database())));
	}
}

boost::shared_ptr<database::connection> database::pool::get()
{
	boost::mutex::scoped_lock lock(Pool_mutex);
	while(Pool.empty()){
		Pool_cond.wait(Pool_mutex);
	}
	boost::shared_ptr<connection> Connection = Pool.top();
	Pool.pop();
	return Connection;
}

database::pool::proxy database::pool::get_proxy()
{
	return proxy();
}

void database::pool::put(boost::shared_ptr<database::connection> & Connection)
{
	boost::mutex::scoped_lock lock(Pool_mutex);
	Pool.push(Connection);
	Pool_cond.notify_one();
}

void database::pool::unit_test_override(const std::string & DB_path)
{
	boost::mutex::scoped_lock lock(Pool_mutex);
	while(!Pool.empty()){
		Pool.pop();
	}
	for(int x=0; x<settings::DATABASE_POOL_SIZE; ++x){
		Pool.push(boost::shared_ptr<connection>(new connection(DB_path)));
	}
}
