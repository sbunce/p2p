//std
#include <fstream>

#include "download_prep.h"

download_prep::download_prep()
{

}

void download_prep::init(volatile bool * download_complete_in)
{
	download_complete = download_complete_in;
}

download * download_prep::start_file(DB_access::download_info_buffer & info)
{
	//make sure file isn't already downloading
	if(!info.resumed){
		if(!DB_Access.download_start_download(info)){
			return NULL;
		}
	}

	//get file path, stop if file not found
	std::string file_path;
	if(!DB_Access.download_get_file_path(info.hash, file_path)){
		return NULL;
	}

	//create an empty file for this download, if a file doesn't already exist
	std::fstream fin(file_path.c_str(), std::ios::in);
	if(fin.is_open()){
		fin.close();
	}
	else{
		std::fstream fout(file_path.c_str(), std::ios::out);
		fout.close();
	}

	unsigned long file_size = strtoul(info.file_size.c_str(), NULL, 10);
	unsigned int latest_request = info.latest_request;

	//(global::P_BLS_SIZE - 1) because control size is 1 byte
	unsigned int last_block = atol(info.file_size.c_str())/(global::P_BLS_SIZE - 1);
	unsigned int last_block_size = atol(info.file_size.c_str()) % (global::P_BLS_SIZE - 1) + 1;

	download * Download = new download_file(
		info.hash,
		info.file_name,
		file_path,
		file_size,
		latest_request,
		last_block,
		last_block_size,
		download_complete
	);

	return Download;
}
