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

unsigned slot::download_peers()
{
	if(Transfer){
		return Transfer->outgoing_count();
	}else{
		return 0;
	}
}

unsigned slot::download_speed()
{
	if(Transfer){
		return Transfer->download_speed();
	}else{
		return 0;
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

const std::string & slot::name()
{
	return file_name;
}

std::string slot::path()
{
	return FI.path;
}

unsigned slot::percent_complete()
{
	if(Transfer){
		return Transfer->percent_complete();
	}else{
		return 0;
	}
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

void slot::touch()
{
	if(Transfer){
		Transfer->touch();
	}
}

unsigned slot::upload_peers()
{
	if(Transfer){
		return Transfer->incoming_count();
	}else{
		return 0;
	}
}

unsigned slot::upload_speed()
{
	if(Transfer){
		return Transfer->upload_speed();
	}else{
		return 0;
	}
}
