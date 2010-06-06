#include "slot.hpp"

slot::slot(const file_info & FI_in):
	FI(FI_in),
	#ifdef _WIN32
	file_name(FI.path.rfind('\\') != std::string::npos ?
		FI.path.substr(FI.path.rfind('\\')+1) : "ERROR")
	#else
	file_name(FI.path.rfind('/') != std::string::npos ?
		FI.path.substr(FI.path.rfind('/')+1) : "ERROR")
	#endif
{
	if(FI.file_size != 0){
		try{
			Transfer.reset(new transfer(FI));
		}catch(std::exception & e){
			LOG << e.what();
			Transfer.reset();
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

boost::optional<boost::uint64_t> slot::file_size()
{
	if(Transfer){
		return Transfer->file_size();
	}else{
		return boost::optional<boost::uint64_t>();
	}
}

const boost::shared_ptr<transfer> & slot::get_transfer()
{
	return Transfer;
}

const std::string & slot::hash()
{
	return FI.hash;
}

p2p::transfer_info slot::info()
{
	p2p::transfer_info tmp;
	tmp.hash = FI.hash;
	tmp.name = file_name;
	if(Transfer){
		tmp.tree_size = Transfer->tree_size();
		tmp.file_size = Transfer->file_size();
		tmp.percent_complete = Transfer->percent_complete();
		tmp.tree_percent_complete = Transfer->tree_percent_complete();
		tmp.file_percent_complete = Transfer->file_percent_complete();
		tmp.download_peers = Transfer->download_count();
		tmp.download_speed = Transfer->download_speed();
		tmp.upload_peers = Transfer->upload_count();
		tmp.upload_speed = Transfer->upload_speed();
	}else{
		tmp.tree_size = 0;
		tmp.file_size = 0;
		tmp.percent_complete = 0;
		tmp.tree_percent_complete = 0;
		tmp.file_percent_complete = 0;
		tmp.download_peers = 0;
		tmp.download_speed = 0;
		tmp.upload_peers = 0;
		tmp.upload_speed = 0;
	}
	return tmp;
}

const std::string & slot::name()
{
	return file_name;
}

std::string slot::path()
{
	return FI.path;
}

bool slot::set_unknown(const int connection_ID, const boost::uint64_t file_size,
	const std::string & root_hash)
{
	boost::mutex::scoped_lock lock(Transfer_mutex);
	if(!Transfer){
		try{
			Transfer.reset(new transfer(FI));
		}catch(std::exception & e){
			LOG << e.what();
			Transfer.reset();
			return false;
		}
		//set file size in case we don't know it
		db::table::share::update_file_size(FI.path, file_size);
	}
	net::buffer buf(convert::hex_to_bin(root_hash));
	Transfer->write_tree_block(connection_ID, 0, buf);
	return true;
}
