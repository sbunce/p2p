#include <fstream>
#include <iostream>
#include <sstream>

#include "digest_DB.h"

digest_DB::digest_DB()
{
	//check if the index file exists, if it doesn't then create it
	std::ifstream index_fstream(global::MESSAGE_DIGEST_INDEX.c_str());

	if(index_fstream.is_open()){
		std::string temp;
		getline(index_fstream, temp);

		//read in existing database information
		index_RecordCount = atoi(temp.substr(0, global::RRN_SIZE).c_str());
		index_RecordFree = atoi(temp.substr(global::RRN_SIZE, global::RRN_SIZE).c_str());

		index_fstream.close();
	}
	else{
		std::ofstream index_fstream(global::MESSAGE_DIGEST_INDEX.c_str());

		if(index_fstream.is_open()){
			//prepare default DB_RecordCount (0)
			std::string index_RecordCount_temp = "1";
			index_RecordCount_temp.append(global::RRN_SIZE - index_RecordCount_temp.size(), ' ');

			//prepare default DB_RecordFree (-1)
			std::string index_RecordFree_temp = "-1";
			index_RecordFree_temp.append(global::RRN_SIZE - index_RecordFree_temp.size(), ' ');

			//combine and write
			std::string combine = index_RecordCount_temp + index_RecordFree_temp;
			combine.append((global::DB_RECORD_SIZE - 1) - combine.size(), ' ');
			index_fstream << combine << "\n";
			index_fstream.close();

			//set the DB_RecordFree and DB_RecordCount data members
			index_RecordCount = 1;
			index_RecordFree = -1;

			index_fstream.close();
		}
		else{
#ifdef DEBUG
			std::cout << "error: digest_DB ctor couldn't create " << global::MESSAGE_DIGEST_INDEX << "\n";
#endif
		}
	}

	//check if the DB file exists, if it doesn't then create it
	std::ifstream DB_fstream(global::MESSAGE_DIGEST_DB.c_str());

	if(DB_fstream.is_open()){
		std::string temp;
		getline(DB_fstream, temp);

		//read in existing database information
		DB_RecordCount = atoi(temp.substr(0, global::RRN_SIZE).c_str());
		DB_RecordFree = atoi(temp.substr(global::RRN_SIZE, global::RRN_SIZE).c_str());

		DB_fstream.close();
	}
	else{ //create index and set defaults
		std::ofstream DB_fstream(global::MESSAGE_DIGEST_DB.c_str());

		if(DB_fstream.is_open()){
			//prepare default DB_RecordCount (0)
			std::string DB_RecordCount_temp = "1";
			DB_RecordCount_temp.append(global::RRN_SIZE - DB_RecordCount_temp.size(), ' ');

			//prepare default DB_RecordFree (-1)
			std::string DB_RecordFree_temp = "-1";
			DB_RecordFree_temp.append(global::RRN_SIZE - DB_RecordFree_temp.size(), ' ');

			//combine and write
			std::string combine = DB_RecordCount_temp + DB_RecordFree_temp;
			combine.append((global::DB_RECORD_SIZE - 1) - combine.size(), ' ');
			DB_fstream << combine << "\n";
			DB_fstream.close();

			//set the DB_RecordFree and DB_RecordCount data members
			DB_RecordCount = 1;
			DB_RecordFree = -1;

			DB_fstream.close();
		}
		else{
#ifdef DEBUG
			std::cout << "error: digest_DB ctor couldn't create " << global::MESSAGE_DIGEST_DB << "\n";
#endif
		}
	}
}

bool digest_DB::addDigests(std::string key, std::deque<std::string> & digests)
{
	std::string temp; //read buffer

	//if the list was located exit out, don't add it again
	if(locateList(key, temp)){
		return false;
	}

	std::fstream DB_fstream(global::MESSAGE_DIGEST_DB.c_str());
	std::string previous_RRN; //stores the RRN for each iteration to link records
	int end_RRN = -1;         //the RRN of the end of this list

	for(std::deque<std::string>::iterator iter0 = digests.begin(); iter0 != digests.end(); iter0++){

		if(DB_RecordFree == -1){

			//seek to the end of the file
			DB_fstream.seekp(0, std::ios::end);

			//write the record
			std::string record;
			if(iter0 == digests.begin()){
				record = *iter0 + global::RECORD_LIST_END;
			}
			else{
				record = *iter0 + previous_RRN;
			}
			record.append((global::DB_RECORD_SIZE - 1) - record.size(), ' ');
			DB_fstream << record << "\n";

			if(end_RRN == -1 && iter0 == digests.end() - 1){
				end_RRN = DB_RecordCount - 1;
			}

			//save the RRN for the next iteration
			std::ostringstream previous_RRN_s;
			previous_RRN_s << DB_RecordCount;
			previous_RRN = previous_RRN_s.str();

			//update DB_RecordCount for next iteration
			DB_RecordCount++;
		}
		else{ //write to available record space

			//seek to the RRN of the free record
			DB_fstream.seekp((DB_RecordFree * global::DB_RECORD_SIZE) + global::MESSAGE_DIGEST_SIZE, std::ios::beg);

			//read the RRN
			std::string next_DB_RecordFree;
			read_RRN(DB_fstream, next_DB_RecordFree);

			//re-seek back to the free record after the last get operation
			DB_fstream.seekp(DB_RecordFree * global::DB_RECORD_SIZE, std::ios::beg);

			//write the record
			std::string record;
			if(iter0 == digests.begin()){
				record = *iter0 + global::RECORD_LIST_END;
			}
			else{
				record = *iter0 + previous_RRN;
			}
			record.append((global::DB_RECORD_SIZE - 1) - record.size(), ' ');
			DB_fstream << record << "\n";

			//save the RRN for the next iteration
			std::ostringstream previous_RRN_s;
			previous_RRN_s << DB_RecordFree;
			previous_RRN = previous_RRN_s.str();

			//if the free record space is used up
			if(next_DB_RecordFree == global::RECORD_LIST_END){
				DB_RecordFree = -1;
			}
			else{ //set next free record, don't increment DB_RecordCount since overwriting records
				DB_RecordFree = atoi(next_DB_RecordFree.c_str());
			}
		}
	}

	DB_fstream.close();

	//update the index
	addKeyToIndex(key, end_RRN);
	update_DB_RecordFree();
	update_DB_RecordCount();

	return true;
}

void digest_DB::addKeyToIndex(std::string key, int RRN)
{
	std::fstream index_fstream(global::MESSAGE_DIGEST_INDEX.c_str());
	index_fstream.seekp(0, std::ios::end);

	std::ostringstream record_s;
	record_s << key << RRN;
	std::string record = record_s.str();
	record.append((global::DB_RECORD_SIZE - 1) - record.size(), ' ');

	index_fstream << record << "\n";
	index_RecordCount++;
	index_fstream.close();

	update_index_RecordCount();
}

bool digest_DB::deleteDigests(std::string key)
{
	std::string RRN_temp;  //temp read buffer
	int next_RRN = -1;     //holds next RRN to be visited

	//holds the original DB_RecordFree to link this list to
	int DB_RecordFreeStart = DB_RecordFree;

	//locate the list to delete, exit out if it doesn't exist
	if(locateList(key, RRN_temp)){
		next_RRN = atoi(RRN_temp.c_str());
		DB_RecordFree = next_RRN;
	}
	else{
		return false;
	}

	std::fstream DB_fstream(global::MESSAGE_DIGEST_DB.c_str());

	while(true){

		//seek to the RRN of the first record in the list
		DB_fstream.seekp((next_RRN * global::DB_RECORD_SIZE) + global::MESSAGE_DIGEST_SIZE, std::ios::beg);

		read_RRN(DB_fstream, RRN_temp);

		if(RRN_temp == global::RECORD_LIST_END){

			//seek to the RRN of the next record in the list
			DB_fstream.seekp((next_RRN * global::DB_RECORD_SIZE) + global::MESSAGE_DIGEST_SIZE, std::ios::beg);

			if(DB_RecordFreeStart != -1){
				//link the DB_RecordFree list with the end of the deleted record list
				std::ostringstream DB_RecordFreeStartLink_s;
				DB_RecordFreeStartLink_s << DB_RecordFreeStart;
				std::string DB_RecordFreeStartLink = DB_RecordFreeStartLink_s.str();
				DB_RecordFreeStartLink.append(global::RRN_SIZE - DB_RecordFreeStartLink.size(), ' ');
				DB_fstream << DB_RecordFreeStartLink;
			}
			else{ //use free record space
				std::string DB_RecordFreeStartLink = global::RECORD_LIST_END;
				DB_RecordFreeStartLink.append(global::RRN_SIZE - DB_RecordFreeStartLink.size(), ' ');
				DB_fstream << DB_RecordFreeStartLink;
			}

			break;
		}
		else{
			//set next_RRN to visit on next iteration
			next_RRN = atoi(RRN_temp.c_str());
		}
	}

	//update the index
	update_DB_RecordFree();
}

bool digest_DB::locateList(std::string key, std::string & RRN)
{
	std::fstream index_fstream(global::MESSAGE_DIGEST_INDEX.c_str());

	//not interested in the first line(DB_RecordCount, DB_RecordFree);
	index_fstream.seekg(global::DB_RECORD_SIZE);

	std::string messageDigest_temp;
	std::string RRN_temp;

	//locate the messageDigest list corresponding to key
	while(true){

		if(!readMessageDigest(index_fstream, messageDigest_temp)){
			return false;
		}

		if(!read_RRN(index_fstream, RRN_temp)){
			return false;
		}

		if(key == messageDigest_temp){
			RRN = RRN_temp;
			index_fstream.close();
			return true;
		}
	}

	index_fstream.close();

	return false;
}

bool digest_DB::readMessageDigest(std::fstream & Index_or_DB, std::string & messageDigest)
{
	messageDigest.clear();

	for(int x=0; x<global::MESSAGE_DIGEST_SIZE; x++){
		messageDigest += Index_or_DB.get();

		if(Index_or_DB.eof()){
			return false;
		}
	}

	return true;
}

bool digest_DB::read_RRN(std::fstream & Index_or_DB, std::string & RRN)
{
	RRN.clear();

	for(int x=0; x<global::RRN_SIZE; x++){
		char ch;
		Index_or_DB.get(ch);

		if(ch == ' '){
			continue;
		}
		else if(Index_or_DB.eof()){
			return false;
		}
		else{
			RRN += ch;
		}
	}

	//if at the end of the record seek past the \n
	if(Index_or_DB.peek() == '\n'){
		Index_or_DB.get();
	}

	return true;
}

bool digest_DB::retrieveDigests(std::string key, std::deque<std::string> & digests)
{
	std::fstream index_fstream(global::MESSAGE_DIGEST_INDEX.c_str());

	std::string temp;  //holds lines from DB or index
	int next_RRN = -1; //holds next RRN to be visited

	//locate the list to delete, exit out if it doesn't exist
	if(locateList(key, temp)){
		next_RRN = atoi(temp.c_str());
	}
	else{
		return false;
	}

	std::fstream DB_fstream(global::MESSAGE_DIGEST_DB.c_str());

	//read the list and copy to digests deque
	while(next_RRN != -1){

		DB_fstream.seekp(next_RRN * global::DB_RECORD_SIZE, std::ios::beg);

		std::string messageDigest_temp;
		readMessageDigest(DB_fstream, messageDigest_temp);
		digests.push_front(messageDigest_temp);

		std::string RRN_temp;
		read_RRN(DB_fstream, RRN_temp);

		if(RRN_temp == global::RECORD_LIST_END){
			next_RRN = -1;
		}
		else{
			next_RRN = atoi(RRN_temp.c_str());
		}
	}

	return true;
}

void digest_DB::update_DB_RecordCount()
{
	std::fstream DB_fstream(global::MESSAGE_DIGEST_DB.c_str());

	std::ostringstream DB_RecordCount_s;
	DB_RecordCount_s << DB_RecordCount;
	DB_RecordCount_s.str().append(global::RRN_SIZE - DB_RecordCount_s.str().size(), ' ');

	DB_fstream << DB_RecordCount_s.str();
	DB_fstream.close();
}

void digest_DB::update_DB_RecordFree()
{
	std::fstream DB_fstream(global::MESSAGE_DIGEST_DB.c_str());
	DB_fstream.seekp(global::RRN_SIZE, std::ios::beg);

	std::ostringstream DB_RecordFree_s;
	DB_RecordFree_s << DB_RecordFree;
	DB_RecordFree_s.str().append(global::RRN_SIZE - DB_RecordFree_s.str().size(), ' ');

	DB_fstream << DB_RecordFree_s.str();
	DB_fstream.close();
}

void digest_DB::update_index_RecordCount()
{
	std::fstream index_fstream(global::MESSAGE_DIGEST_INDEX.c_str());

	std::ostringstream DB_RecordCount_s;
	DB_RecordCount_s << index_RecordCount;
	DB_RecordCount_s.str().append(global::RRN_SIZE - DB_RecordCount_s.str().size(), ' ');

	index_fstream << DB_RecordCount_s.str();
	index_fstream.close();
}

void digest_DB::update_index_RecordFree()
{
	std::fstream index_fstream(global::MESSAGE_DIGEST_INDEX.c_str());
	index_fstream.seekp(global::RRN_SIZE, std::ios::beg);

	std::ostringstream DB_RecordFree_s;
	DB_RecordFree_s << index_RecordFree;
	DB_RecordFree_s.str().append(global::RRN_SIZE - DB_RecordFree_s.str().size(), ' ');

	index_fstream << DB_RecordFree_s.str();
	index_fstream.close();
}

