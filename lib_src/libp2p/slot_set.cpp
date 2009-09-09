#include "slot_set.hpp"

slot_set::slot_set()
{

}


slot_set::~slot_set()
{
	resume_thread.interrupt();
	resume_thread.join();
}

boost::shared_ptr<slot> slot_set::add(const std::string & hash)
{
	boost::mutex::scoped_lock lock(Slot_mutex);
	std::map<std::string, boost::shared_ptr<slot> >::iterator
		iter = Slot.find(hash);
	if(iter == Slot.end()){
		//slot element doesn't exist, look up hash in database
		if(boost::shared_ptr<database::table::share::file_info>
			FI = database::table::share::lookup_hash(hash))
		{
			//file found
			if(FI->State == database::table::share::COMPLETE){
				boost::shared_ptr<slot> SE(new slot(FI->hash, FI->file_size, FI->path));
				Slot.insert(std::make_pair(SE->hash, SE));
			}else{
				/*
				File is downloading. Downloading files may only be added by resume.
				We don't return it here because it could be hash checking.
				*/
				return boost::shared_ptr<slot>();
			}
		}else{
			return boost::shared_ptr<slot>();
		}
	}else{
		//slot_element already exists
		return iter->second;
	}
}

void slot_set::remove(const std::string & hash)
{
	boost::mutex::scoped_lock lock(Slot_mutex);
	Slot.erase(hash);
}

void slot_set::resume(network::proactor & Proactor)
{
	resume_thread = boost::thread(boost::bind(&slot_set::resume_priv, this,
		boost::ref(Proactor)));
}

void slot_set::resume_priv(network::proactor & Proactor)
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
	boost::mutex::scoped_lock lock(Slot_mutex);
	for(std::vector<database::table::share::file_info>::iterator iter_cur = Resume.begin(),
		iter_end = Resume.end(); iter_cur != iter_end; ++iter_cur)
	{
		boost::shared_ptr<slot> SE(new slot(iter_cur->hash, iter_cur->file_size,
			iter_cur->path));
		Slot_Resume.insert(std::make_pair(SE->hash, SE));
	}
	}//end lock scope

	while(true){
		boost::shared_ptr<slot> SE;

		{//begin lock scope
		boost::mutex::scoped_lock lock(Slot_mutex);
		if(Slot_Resume.empty()){
			break;
		}else{
			SE = Slot_Resume.begin()->second;
		}
		}//end lock scope

		//hash check
		SE->Hash_Tree.check();

		//move to Slot_Element container when complete
		{//begin lock scope
		boost::mutex::scoped_lock lock(Slot_mutex);
		//only move to Slot_Element if it wasn't cancelled
		if(Slot_Resume.erase(SE->hash) == 1){
			Slot.insert(std::make_pair(SE->hash, SE));
		}
		}//end lock scope

		//initiate connections for file
		if(boost::shared_ptr<std::vector<database::table::host::host_info> >
			HI = database::table::host::lookup(SE->hash))
		{
			for(std::vector<database::table::host::host_info>::iterator
				iter_cur = HI->begin(), iter_end = HI->end(); iter_cur != iter_end;
				++iter_cur)
			{
				Proactor.connect(iter_cur->host, iter_cur->port);
				Proactor.trigger_call_back(iter_cur->host, iter_cur->port);
			}
		}
	}
}
