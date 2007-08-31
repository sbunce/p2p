#include <bitset>
#include <iostream>

#include "clientBuffer.h"

clientBuffer::clientBuffer(const std::string & server_IP_in, int * sendPending_in)
{
	server_IP = server_IP_in;
	sendPending = sendPending_in;
	bytesExpected = 0;
	latestRequested = 0;
	recvBuffer.reserve(global::BUFFER_SIZE);
	sendBuffer.reserve(global::C_CTRL_SIZE);
	Download_iter = Download.begin();
	ready = true;
	abusive = false;
}

void clientBuffer::addDownload(const unsigned int & file_ID, download * newDownload)
{
	downloadHolder DownloadHolder(file_ID, newDownload);
	Download.push_back(DownloadHolder);
}

std::string clientBuffer::encodeInt(const unsigned int & number)
{
	std::string encodedNumber;
	std::bitset<32> bs = number;
	std::bitset<32> bs_temp;

	for(int x=0; x<4; x++){
		bs_temp = bs;
		bs_temp <<= 8*x;
		bs_temp >>= 24;
		encodedNumber += (unsigned char)bs_temp.to_ulong();
	}

	return encodedNumber;
}

const std::string & clientBuffer::get_IP()
{
	return server_IP;
}

void clientBuffer::postRecv()
{
	if(recvBuffer.size() == bytesExpected){
		Download_iter->Download->addBlock(latestRequested, recvBuffer);
		recvBuffer.clear();
		ready = true;
	}

	if(recvBuffer.size() > bytesExpected){
		abusive = true;
	}
}

void clientBuffer::postSend()
{
	if(sendBuffer.empty()){
		(*sendPending)--;
	}
}

void clientBuffer::prepareRequest()
{
	if(ready){
		rotateDownloads();
		latestRequested = Download_iter->Download->getRequest();
		bytesExpected = Download_iter->Download->getBytesExpected();
		sendBuffer = global::P_SBL + encodeInt(Download_iter->file_ID) + encodeInt(latestRequested);
		(*sendPending)++;
		ready = false;
	}
}

void clientBuffer::rotateDownloads()
{
	if(!Download.empty()){
		Download_iter++;
	}

	if(Download_iter == Download.end()){
		Download_iter = Download.begin();
	}
}

