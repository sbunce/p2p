#include "slot.hpp"

slot::slot(
	const file_info & FI
):
	Hash_Tree(FI),
	File(FI)
{

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
	return Hash_Tree.complete() && File.complete();
}

unsigned slot::download_speed()
{
	return 0;
}

boost::uint64_t slot::file_size()
{
	return File.file_size;
}

const std::string & slot::hash()
{
	return Hash_Tree.hash;
}

std::string slot::name()
{
	int pos;
	if((pos = File.path.rfind('/')) != std::string::npos){
		return File.path.substr(pos+1);
	}else{
		return "";
	}
}

unsigned slot::percent_complete()
{
	return 69;
}

unsigned slot::upload_speed()
{
	return 0;
}

/*
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
*/
