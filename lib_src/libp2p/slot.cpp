#include "slot.hpp"

slot::slot(const file_info & FI_in):
	FI(FI_in),
	file_name(FI.path.get().rfind('/') != std::string::npos ?
		FI.path.get().substr(FI.path.get().rfind('/')+1) : "ERROR")
{
	if(FI.file_size != 0){
		try{
			Transfer = boost::shared_ptr<transfer>(new transfer(FI));
		}catch(std::exception & ex){
			LOGGER << ex.what();
			Transfer = boost::shared_ptr<transfer>();
		}
	}
}

bool slot::complete()
{
	if(Transfer){
		return Transfer->complete();
	}else{
		return false;
	}
}

unsigned slot::download_speed()
{
	if(Transfer){
//DEBUG, implement granular rate measurement later
		return 0;
	}else{
		return 0;
	}
}

boost::uint64_t slot::file_size()
{
	boost::mutex::scoped_lock lock(FI_mutex);
	return FI.file_size;
}

const boost::shared_ptr<transfer> & slot::get_transfer()
{
	return Transfer;
}

const std::string & slot::hash()
{
	boost::mutex::scoped_lock lock(FI_mutex);
	return FI.hash;
}

const std::string & slot::name()
{
	boost::mutex::scoped_lock lock(FI_mutex);
	return file_name;
}

unsigned slot::percent_complete()
{
	if(Transfer){
//DEBUG, implement later
		return 25;
	}else{
		return 25;
	}
}

bool slot::set_unknown(const boost::uint64_t file_size, const std::string & root_hash)
{
	//true if this thread instantiated transfer (used to save disk access)
	bool created = false;
	std::string path_tmp;

	{//BEGIN lock scope
	boost::mutex::scoped_lock lock(FI_mutex);
	if(!Transfer){
		created = true;
		FI.file_size = file_size;
		path_tmp = FI.path;
		try{
			Transfer = boost::shared_ptr<transfer>(new transfer(FI));
		}catch(std::exception & ex){
			LOGGER << ex.what();
			Transfer = boost::shared_ptr<transfer>();
			return false;
		}
	}
	}//END lock scope
//DEBUG, disable this optimization until resume hash checking working
//	if(created){
		database::table::share::update_file_size(path_tmp, file_size);
		network::buffer buf(convert::hex_to_bin(root_hash));
		Transfer->write_tree_block(0, buf);
//	}
	return true;
}

unsigned slot::upload_speed()
{
	if(Transfer){
//DEBUG, implement granular rate measurement later
		return 0;
	}else{
		return 0;
	}
}
