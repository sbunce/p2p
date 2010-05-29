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

boost::uint64_t slot::file_size()
{
	if(Transfer){
		return Transfer->file_size();
	}else{
		return FI.file_size;
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

p2p::transfer slot::info()
{
	p2p::transfer transfer;
	transfer.hash = FI.hash;
	transfer.name = file_name;
	if(Transfer){
		transfer.percent_complete = Transfer->percent_complete();
		transfer.download_peers = Transfer->outgoing_count();
		transfer.download_speed = Transfer->download_speed();
		transfer.upload_peers = Transfer->incoming_count();
		transfer.upload_speed = Transfer->upload_speed();
	}else{
		transfer.percent_complete = 0;
		transfer.download_peers = 0;
		transfer.download_speed = 0;
		transfer.upload_peers = 0;
		transfer.upload_speed = 0;
	}
	return transfer;
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
