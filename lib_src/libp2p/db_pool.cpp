#include "db_pool.hpp"

//BEGIN pool::proxy
db::pool::proxy::proxy():
	Connection(singleton()->pool_get()),
	This(this, boost::bind(&proxy::deleter, this, Connection))
{

}

boost::shared_ptr<db::connection> & db::pool::proxy::operator -> ()
{
	return Connection;
}

void db::pool::proxy::deleter(boost::shared_ptr<db::connection> Connection)
{
	singleton()->pool_put(Connection);
}
//END pool::proxy

db::pool::pool()
{
	for(int x=0; x<settings::DATABASE_POOL_SIZE; ++x){
		Pool.push(boost::shared_ptr<connection>(new connection(path::db_file())));
	}
}

boost::shared_ptr<db::connection> db::pool::pool_get()
{
	boost::mutex::scoped_lock lock(Pool_mutex);
	while(Pool.empty()){
		Pool_cond.wait(Pool_mutex);
	}
	boost::shared_ptr<connection> Connection = Pool.top();
	Pool.pop();
	return Connection;
}

db::pool::proxy db::pool::get()
{
	return proxy();
}

void db::pool::pool_put(boost::shared_ptr<db::connection> & Connection)
{
	boost::mutex::scoped_lock lock(Pool_mutex);
	Pool.push(Connection);
	Pool_cond.notify_one();
}
