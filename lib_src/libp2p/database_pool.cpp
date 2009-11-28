#include "database_pool.hpp"

//BEGIN pool::proxy
database::pool::proxy::proxy():
	Connection(singleton().pool_get()),
	This(this, boost::bind(&proxy::deleter, this, Connection))
{

}

boost::shared_ptr<database::connection> & database::pool::proxy::operator -> ()
{
	return Connection;
}

void database::pool::proxy::deleter(boost::shared_ptr<database::connection> Connection)
{
	singleton().pool_put(Connection);
}
//END pool::proxy

database::pool::pool()
{
	for(int x=0; x<settings::DATABASE_POOL_SIZE; ++x){
		Pool.push(boost::shared_ptr<connection>(new connection(path::database())));
	}
}

boost::shared_ptr<database::connection> database::pool::pool_get()
{
	boost::mutex::scoped_lock lock(Pool_mutex);
	while(Pool.empty()){
		Pool_cond.wait(Pool_mutex);
	}
	boost::shared_ptr<connection> Connection = Pool.top();
	Pool.pop();
	return Connection;
}

database::pool::proxy database::pool::get()
{
	return proxy();
}

void database::pool::pool_put(boost::shared_ptr<database::connection> & Connection)
{
	boost::mutex::scoped_lock lock(Pool_mutex);
	Pool.push(Connection);
	Pool_cond.notify_one();
}
