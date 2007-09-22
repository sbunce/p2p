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
	abuse = false;
	terminating = false;

	lastSeen = time(0);
}

void clientBuffer::addDownload(const unsigned int & file_ID, download * newDownload)
{
	downloadHolder DownloadHolder(file_ID, newDownload);
	Download.push_back(DownloadHolder);
	newDownload->regConnection(server_IP);
}

const bool clientBuffer::empty()
{
	return Download.empty();
}

std::string clientBuffer::encodeInt(const unsigned int & number)
{
	std::string encodedNumber;
	std::bitset<32> bs = number;
	std::bitset<32> bs_temp;

	for(int x=0; x<4; ++x){
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

const time_t & clientBuffer::get_lastSeen()
{
	return lastSeen;
}

void clientBuffer::postRecv()
{
	if(recvBuffer.size() == bytesExpected){
		Download_iter->Download->addBlock(latestRequested, recvBuffer);
		recvBuffer.clear();
		ready = true;
	}

	if(recvBuffer.size() > bytesExpected){
		abuse = true;
	}

	lastSeen = time(0);
}

void clientBuffer::postSend()
{
	if(sendBuffer.empty()){
		--(*sendPending);
	}
}

void clientBuffer::prepareRequest()
{
	if(ready && !terminating){
		rotateDownloads();
		latestRequested = Download_iter->Download->getRequest();
		bytesExpected = Download_iter->Download->getBytesExpected();
		sendBuffer = global::P_SBL + encodeInt(Download_iter->file_ID) + encodeInt(latestRequested);
		++(*sendPending);
		ready = false;
	}
}

void clientBuffer::rotateDownloads()
{
	if(!Download.empty()){
		++Download_iter;
	}

	if(Download_iter == Download.end()){
		Download_iter = Download.begin();
	}
}

const bool clientBuffer::terminateDownload(const std::string & hash)
{
	//if currently on this download then still expecting bytes
	if(hash == Download_iter->Download->getHash()){
		if(ready){ //if not expecting any more bytes
			Download_iter->Download->unregConnection(server_IP);
			Download_iter = Download.erase(Download_iter);
			terminating = false;
			return true;
		}
		else{ //expecting more bytes, let them finish before terminating
			terminating = true;
			return false;
		}
	}

	std::list<downloadHolder>::iterator iter_cur, iter_end;
	iter_cur = Download.begin();
	iter_end = Download.end();
	while(iter_cur != iter_end){
		if(hash == iter_cur->Download->getHash()){
			iter_cur->Download->unregConnection(server_IP);
			Download.erase(iter_cur);
			break;
		}
		++iter_cur;
	}

	return true;
}

void clientBuffer::unregisterAll()
{
	std::list<downloadHolder>::iterator iter_cur, iter_end;
	iter_cur = Download.begin();
	iter_end = Download.end();
	while(iter_cur != iter_end){
		iter_cur->Download->unregConnection(server_IP);
		++iter_cur;
	}
}

