#include "ID_manager.hpp"

network::ID_manager::ID_manager():
	highest_allocated(0)
{

}

int network::ID_manager::allocate()
{
	boost::mutex::scoped_lock lock(Mutex);
	while(true){
		++highest_allocated;
		if(allocated.find(highest_allocated) == allocated.end()){
			allocated.insert(highest_allocated);
			return highest_allocated;
		}
	}
	LOG;
	exit(1);
}

void network::ID_manager::deallocate(int connection_ID)
{
	boost::mutex::scoped_lock lock(Mutex);
	if(allocated.erase(connection_ID) != 1){
		LOG << "double free of connection_ID";
		exit(1);
	}
}
