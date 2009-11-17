#include "slot.hpp"

slot::slot(
	const file_info & FI_in
):
	FI(FI_in)
{
//DEBUG, check to see if hash tree and file can be instantiated

//	if(FI.hash
	host = database::table::host::lookup(FI.hash);
}

void slot::check()
{
/*
	Hash_Tree.check();
	if(Hash_Tree.complete()){

	}
*/
}

bool slot::complete()
{
	if(Hash_Tree && File){
		return Hash_Tree->complete() && File->complete();
	}else{
		return false;
	}
}

unsigned slot::download_speed()
{
	return 0;
}

boost::uint64_t slot::file_size()
{
	return FI.file_size;
}

bool slot::has_file(const std::string & IP, const std::string & port)
{
	boost::mutex::scoped_lock lock(host_mutex);
	return host.find(std::make_pair(IP, port)) != host.end();
}

const std::string & slot::hash()
{
	return FI.hash;
}

void slot::merge_host(std::set<std::pair<std::string, std::string> > & host_in)
{
	boost::mutex::scoped_lock lock(host_mutex);
	host_in.insert(host.begin(), host.end());
}

std::string slot::name()
{
	int pos;
	if((pos = FI.path.get().rfind('/')) != std::string::npos){
		return FI.path.get().substr(pos+1);
	}else{
		return "";
	}
}

unsigned slot::percent_complete()
{
	return 69;
}

//DEBUG, this should take Send_Queue ref as parameter?
bool slot::recv(const network::buffer & recv_buf)
{

}

bool slot::slot_ID(network::buffer & send_buf, const unsigned char slot_num)
{
	assert(send_buf.empty());
	if(!Hash_Tree || !File){
		return false;
	}

	unsigned char status;
	if(Hash_Tree->complete() && File->complete()){
		status = 0;
	}else if(Hash_Tree->complete() && !File->complete()){
		status = 1;
	}else if(!Hash_Tree->complete() && File->complete()){
		status = 2;
	}else if(!Hash_Tree->complete() && !File->complete()){
		status = 3;
	}
	
	send_buf.append(protocol::SLOT_ID)
		.append(slot_num)
		.append(status)
		.append(convert::encode(file_size()));
	send_buf.reserve(SHA1::bin_size);
	send_buf.resize(SHA1::bin_size);
	if(!database::pool::get()->blob_read(Hash_Tree->blob,
		reinterpret_cast<char *>(send_buf.tail_start()), SHA1::bin_size, 0))
	{
		LOGGER << "error reading hash tree";
		return false;
	}

	//make sure file size and root hash are valid
	SHA1 SHA;
	SHA.init();
	SHA.load(reinterpret_cast<const char *>(send_buf.data()) + 3,
		8 + SHA1::bin_size);
	SHA.end();
	if(SHA.hex() == hash()){
		return true;
	}else{
		LOGGER << "failed hash check";
		return false;
	}
}

unsigned slot::upload_speed()
{
	return 0;
}

/*
void slot_manager::send_request_slot(const std::string & hash)
{
	boost::shared_ptr<message> M(new message());
	M->send_buf.append(protocol::REQUEST_SLOT).append(convert::hex_to_bin(hash));
	M->expected_response.push_back(std::make_pair(protocol::SLOT_ID,
		protocol::SLOT_ID_SIZE));
	M->expected_response.push_back(std::make_pair(protocol::REQUEST_SLOT_FAILED,
		protocol::REQUEST_SLOT_FAILED_SIZE));
	Send_Queue.push_back(M);
}

void slot_manager::send_request_slot_failed()
{
	boost::shared_ptr<message> M(new message());
	M->send_buf.append(protocol::REQUEST_SLOT_FAILED);
	Send_Queue.push_back(M);
}

void slot_manager::send_request_block(
	std::pair<unsigned char, boost::shared_ptr<slot> > & P)
{

	//request hash tree blocks first
	boost::uint64_t block;
	if(P.second->Hash_Tree.Block_Request.next_request(connection_ID, block)){
		boost::shared_ptr<message> M(new message());
		M->Slot = P.second;
		M->send_buf.append(protocol::REQUEST_BLOCK_HASH_TREE).append(P.first)
			.append(convert::encode_VLI(block, P.second->Hash_Tree.tree_block_count));
		M->expected_response.push_back(std::make_pair(protocol::BLOCK,
			protocol::BLOCK_SIZE(P.second->Hash_Tree.block_size(block))));
		M->expected_response.push_back(std::make_pair(protocol::FILE_REMOVED,
			protocol::FILE_REMOVED_SIZE));
		Send_Queue.push_back(M);
	}else if(P.second->File.Block_Request.next_request(connection_ID, block)){
		boost::shared_ptr<message> M(new message());
		M->Slot = P.second;
		M->send_buf.append(protocol::REQUEST_BLOCK_FILE).append(P.first)
			.append(convert::encode_VLI(block, P.second->File.block_count));
		M->expected_response.push_back(std::make_pair(protocol::BLOCK,
			protocol::BLOCK_SIZE(P.second->File.block_size(block))));
		M->expected_response.push_back(std::make_pair(protocol::FILE_REMOVED,
			protocol::FILE_REMOVED_SIZE));
		Send_Queue.push_back(M);
	}
}

void slot_manager::send_file_removed()
{
	boost::shared_ptr<message> M(new message());
	M->send_buf.append(protocol::FILE_REMOVED);
	Send_Queue.push_back(M);
}
*/
