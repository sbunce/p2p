#include "slot.hpp"

slot::slot(const file_info & FI_in):
	_name(FI_in.path.get().rfind('/') == std::string::npos ?
		FI_in.path.get().substr(FI_in.path.get().rfind('/')+1) : "ERROR")
{
	if(FI_in.file_size == 0){
		FI = boost::shared_ptr<file_info>(new file_info(FI_in));
	}else{
		Transfer = boost::shared_ptr<transfer>(new transfer(FI_in));
	}
}

void slot::check()
{
	if(Transfer){
		Transfer->check();
	}
}

bool slot::complete()
{
	if(FI){
		return false;
	}else{
		return Transfer->complete();
	}
}

unsigned slot::download_speed()
{
//DEBUG, implement granular rate measurement later
	return 0;
}

boost::uint64_t slot::file_size()
{
	if(FI){
		return FI->file_size;
	}else{
		return Transfer->file_size();
	}
}

const std::string & slot::hash()
{
	if(FI){
		return FI->hash;
	}else{
		return Transfer->hash();
	}
}

const std::string & slot::name()
{
	return _name;
}

unsigned slot::percent_complete()
{
//DEBUG, implement later
	return 69;
}

bool slot::root_hash(std::string & RH)
{
	if(Transfer){
		return Transfer->root_hash(RH);
	}else{
		return false;
	}
}

void slot::set_unknown(const boost::uint64_t file_size, const std::string & root_hash)
{
	if(!Transfer){
		assert(FI);
		FI->file_size = file_size;
		Transfer = boost::shared_ptr<transfer>(new transfer(*FI));
		FI = boost::shared_ptr<file_info>();
		network::buffer buf(convert::hex_to_bin(root_hash));
		Transfer->write_hash_tree_block(0, buf);
	}
}

bool slot::status(unsigned char & byte)
{
	if(Transfer){
		return Transfer->status(byte);
	}else{
		return false;
	}
}

unsigned slot::upload_speed()
{
//DEBUG, implement granular rate measurement later
	return 0;
}
