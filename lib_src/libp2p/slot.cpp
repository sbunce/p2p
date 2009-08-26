#include "slot.hpp"

slot::slot()
{

}

bool slot::add_incoming_slot()
{
/*
	if(Incoming_Slot.size() >= 256){
		//violated protocol by opening too many slots
		LOGGER << "remote host opened more than 256 slots";
		return false;
	}

	//check for unused allocated slot
	for(int x=0, end=Incoming_Slot.size(); x<end; ++x){
		if(!Incoming_Slot[x]){
			Incoming_Slot[x] = SE;
			send_buff.append(protocol::SLOT_ID).append(static_cast<unsigned char>(x));
			return true;
		}
	}

	//no unused allocated slot found, allocate new slot
	Incoming_Slot.push_back(SE);
	send_buff.append(protocol::SLOT_ID)
		.append(static_cast<unsigned char>(Incoming_Slot.size()-1));
	return true;
*/
}

void slot::add_outgoing_slot(boost::shared_ptr<slot_element> SE)
{
	Pending_Slot_Request.push_back(SE);
}

bool slot::recv(network::buffer & recv_buff)
{

}

bool slot::send(network::buffer & send_buff)
{
//DEBUG, work on this

	return false;
}
