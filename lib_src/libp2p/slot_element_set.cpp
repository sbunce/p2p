#include "slot_element_set.hpp"

slot_element_set::slot_element_set()
{

}

boost::shared_ptr<slot_element> slot_element_set::get(const std::string & hash)
{
	boost::mutex::scoped_lock lock(Slot_Element_mutex);
	std::map<std::string, boost::shared_ptr<slot_element> >::iterator
		iter = Slot_Element.find(hash);
	if(iter == Slot_Element.end()){
		//slot element doesn't exist, look up hash in database


		return boost::shared_ptr<slot_element>();
	}else{
		//slot_element already exists
		return iter->second;
	}
}

void slot_element_set::remove(const std::string & hash)
{
	boost::mutex::scoped_lock lock(Slot_Element_mutex);
	Slot_Element.erase(hash);
}

void slot_element_set::resume(network::proactor & Proactor)
{
	//read all downloading files
	//database::table::share::resume(Resume);

	{//begin lock scope
	boost::mutex::scoped_lock lock(Slot_Element_mutex);

	}//end lock scope
}
