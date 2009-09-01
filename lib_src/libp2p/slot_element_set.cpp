#include "slot_element_set.hpp"

slot_element_set::slot_element_set():
	slot_element_set_state(0)
{

}


slot_element_set::~slot_element_set()
{
	resume_thread.interrupt();
	resume_thread.join();
}

bool slot_element_set::exists(boost::shared_ptr<slot_element> & SE)
{
	boost::mutex::scoped_lock lock(Slot_Element_mutex);
	return Slot_Element.find(SE->hash) != Slot_Element.end();
}

boost::shared_ptr<slot_element> slot_element_set::get(const std::string & hash)
{
	boost::mutex::scoped_lock lock(Slot_Element_mutex);
	std::map<std::string, boost::shared_ptr<slot_element> >::iterator
		iter = Slot_Element.find(hash);
	if(iter == Slot_Element.end()){
		//slot element doesn't exist, look up hash in database
		if(boost::shared_ptr<database::table::share::file_info>
			FI = database::table::share::lookup_hash(hash))
		{
			//file found
			if(FI->State == database::table::share::COMPLETE){
				boost::shared_ptr<slot_element> SE(new slot_element(FI->hash,
					FI->file_size, FI->path));
				Slot_Element.insert(std::make_pair(SE->hash, SE));
				++slot_element_set_state;
			}else{
				/*
				File is downloading. Downloading files may only be added by resume.
				We don't return it here because it could be hash checking.
				*/
				return boost::shared_ptr<slot_element>();
			}
		}else{
			return boost::shared_ptr<slot_element>();
		}
	}else{
		//slot_element already exists
		return iter->second;
	}
}

bool slot_element_set::modified(int & last_state_seen)
{
	if(last_state_seen == slot_element_set_state){
		return false;
	}else{
		last_state_seen = slot_element_set_state;
		return true;
	}
}

void slot_element_set::remove(const std::string & hash)
{
	boost::mutex::scoped_lock lock(Slot_Element_mutex);
	Slot_Element.erase(hash);
	++slot_element_set_state;
}

void slot_element_set::resume(network::proactor & Proactor)
{
	resume_thread = boost::thread(boost::bind(&slot_element_set::resume_priv,
		this, boost::ref(Proactor)));
}

void slot_element_set::resume_priv(network::proactor & Proactor)
{
	//read all downloading file infomation
	std::vector<database::table::share::file_info> Resume;
	database::table::share::resume(Resume);

	/*
	Create slot_elements for each file and store them in the Slot_Element_Resume
	container. We will hash check all of these and move them to the Slot_Element
	container as they finish.
	*/
	{//begin lock scope
	boost::mutex::scoped_lock lock(Slot_Element_mutex);
	for(std::vector<database::table::share::file_info>::iterator iter_cur = Resume.begin(),
		iter_end = Resume.end(); iter_cur != iter_end; ++iter_cur)
	{
		boost::shared_ptr<slot_element> SE(new slot_element(iter_cur->hash,
			iter_cur->file_size, iter_cur->path));
		Slot_Element_Resume.insert(std::make_pair(SE->hash, SE));
	}
	}//end lock scope

	while(true){
		boost::shared_ptr<slot_element> SE;

		{//begin lock scope
		boost::mutex::scoped_lock lock(Slot_Element_mutex);
		if(Slot_Element_Resume.empty()){
			break;
		}else{
			SE = Slot_Element_Resume.begin()->second;
		}
		}//end lock scope

		//hash check
		SE->Hash_Tree.check();

		//move to Slot_Element container when complete
		{//begin lock scope
		boost::mutex::scoped_lock lock(Slot_Element_mutex);
		//only move to Slot_Element if it wasn't cancelled
		if(Slot_Element_Resume.erase(SE->hash) == 1){
			Slot_Element.insert(std::make_pair(SE->hash, SE));
		}
		}//end lock scope

		++slot_element_set_state;

		/*
		Connect to hosts which have the file (the proactor won't connect twice if
		it's already connected). If there is already a connection to the host
		trigger proactor to do call back so the connection can add the
		slot_element.
		*/
		//Proactor.connect("some host name", "some port");
		//Proactor.trigger_call_back("some host name");
	}
}
