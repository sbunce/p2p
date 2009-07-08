//include
#include <database_connection_pool.hpp>

int main()
{
	for(int x=0; x<16; ++x){
		boost::shared_ptr<database::connection> Connection(new database::connection("DB"));
		database::connection_pool::singleton().put(Connection);
	}

	for(int x=0; x<16; ++x){
		database::connection_pool::singleton().get();
	}
}
