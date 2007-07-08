#include <fstream>
#include <iostream>
#include <sstream>

#include "digest_DB.h"

digest_DB::digest_DB()
{
	//check if the index file exists, if it doesn't then create it
	std::ifstream fin(global::MESSAGE_DIGEST_INDEX.c_str());

	if(fin.is_open()){
		std::string temp;
		getline(fin, temp);

		//read in existing database information
		RRN_Free = atoi(temp.substr(0, global::RRN_SIZE).c_str());
		RRN_Count = atoi(temp.substr(global::RRN_SIZE, global::RRN_SIZE).c_str());

		fin.close();
	}
	else{ //create index and set defaults
		std::ofstream index_fstream(global::MESSAGE_DIGEST_INDEX.c_str());

		//write the default RRN_Count to the file (0)
		std::string RRN_CountIndex = "0";
		RRN_CountIndex.append(global::RRN_SIZE - RRN_CountIndex.size(), ' ');
		index_fstream << RRN_CountIndex;

		//write the default RRN_Free to the file (-1)
		std::string RRN_FreeIndex = "-1";
		RRN_FreeIndex.append(global::RRN_SIZE - RRN_FreeIndex.size(), ' ');
		index_fstream << RRN_FreeIndex << "\n";

		//set the RRN_Free and RRN_Count data members
		RRN_Free = -1;
		RRN_Count = 0;

		index_fstream.close();
	}

	//check if the database file exists, if it doesn't then create it
	fin.open(global::MESSAGE_DIGEST_DB.c_str());

	if(fin.is_open()){
		fin.close();
	}
	else{
		std::ofstream fout(global::MESSAGE_DIGEST_DB.c_str());
		fout.close();
	}
}

bool digest_DB::addDigests(std::string key, std::deque<std::string> & digests)
{
	std::fstream index_fstream(global::MESSAGE_DIGEST_INDEX.c_str());
	std::fstream DB_fstream(global::MESSAGE_DIGEST_DB.c_str());

	std::string temp; //holds lines from DB or index

	//not interested in the first line(RRN_Free, RRN_Count);
	index_fstream.seekg(global::RRN_SIZE * 2);

	//check to see if this set of digests already exists
	while(getline(index_fstream, temp)){

		if(key == temp.substr(0, global::MESSAGE_DIGEST_SIZE)){
			index_fstream.close();
			DB_fstream.close();
			return false;
		}
	}

	index_fstream.close();

	std::string previous_RRN; //stores the RRN for each iteration to link records
	int end_RRN = -1;         //the RRN of the end of this list

	for(std::deque<std::string>::iterator iter0 = digests.begin(); iter0 != digests.end(); iter0++){

		if(RRN_Free == -1){

			//seek to the end of the file
			DB_fstream.seekp(0, std::ios::end);

			if(end_RRN == -1 && iter0 == digests.end() - 1){
				end_RRN = RRN_Count;
			}

			//write the record
			std::string record;
			if(iter0 == digests.begin()){
				record = *iter0 + global::RECORD_LIST_END;
			}
			else{
				record = *iter0 + previous_RRN;
			}
			record.append((global::RECORD_SIZE - 1) - record.size(), ' '); //-1 for \n
			DB_fstream << record << "\n";

			//save the RRN for the next iteration
			std::ostringstream previous_RRN_s;
			previous_RRN_s << RRN_Count;
			previous_RRN = previous_RRN_s.str();

			//update RRN_Count for next iteration
			RRN_Count++;
		}
		else{ //write to available record space

			//seek to the RRN of the free record
			DB_fstream.seekp((RRN_Free * global::RECORD_SIZE) + global::MESSAGE_DIGEST_SIZE, std::ios::beg);

			//read the RRN of the free record(pointer to next free record)
			char RRN_ch[global::RRN_SIZE];
			DB_fstream.get(RRN_ch, global::RRN_SIZE, ' ');
			temp.clear();
			temp.assign(RRN_ch, DB_fstream.gcount());
			std::string next_RRN_Free = temp;

			//re-seek back to the free record after the last get operation
			DB_fstream.seekp(RRN_Free * global::RECORD_SIZE, std::ios::beg);

			//write the record
			std::string record;
			if(iter0 == digests.begin()){
				record = *iter0 + global::RECORD_LIST_END;
			}
			else{
				record = *iter0 + previous_RRN;
			}
			record.append((global::RECORD_SIZE - 1) - record.size(), ' ');
			DB_fstream << record << "\n";

			//save the RRN for the next iteration
			std::ostringstream previous_RRN_s;
			previous_RRN_s << RRN_Free;
			previous_RRN = previous_RRN_s.str();

			//if the free record space is used up
			if(next_RRN_Free == global::RECORD_LIST_END){
				RRN_Free = -1;

				//increment RRN_Count for next iteration
				RRN_Count++;
			}
			else{ //set next free record, don't increment RRN_Count since overwriting records
				RRN_Free = atoi(next_RRN_Free.c_str());
			}
		}
	}

	index_fstream.open(global::MESSAGE_DIGEST_INDEX.c_str());

	//update the index
	update_RRN_Free(index_fstream);
	update_RRN_Count(index_fstream);

	//add the key and RRN for the last messageDigest in the list to the index
	index_fstream.seekp(0, std::ios::end);
	std::ostringstream record_s;
	record_s << key << end_RRN;
	std::string record = record_s.str();
	record.append(global::RECORD_SIZE - record.size(), ' ');
	index_fstream << record << "\n";

	index_fstream.close();
	DB_fstream.close();

	return true;
}

bool digest_DB::deleteDigests(std::string key)
{
	std::fstream index_fstream(global::MESSAGE_DIGEST_INDEX.c_str());

	std::string temp;      //holds lines from DB or index
	int next_RRN = -1;     //holds next RRN to be visited

	//holds the original RRN_Free to link this list to
	int RRN_FreeStart = RRN_Free;

	//not interested in the first line(RRN_Free, RRN_Count);
	index_fstream.seekg(global::RRN_SIZE * 2);

	//locate the messageDigest list to delete
	while(getline(index_fstream, temp)){

		if(key == temp.substr(0, global::MESSAGE_DIGEST_SIZE)){

			//store the RRN of the start of the list
			next_RRN = atoi(temp.substr(global::MESSAGE_DIGEST_SIZE, global::RECORD_SIZE - temp.find_first_of(' ', global::MESSAGE_DIGEST_SIZE)).c_str());

			//RRN_Free list now starts here
			RRN_Free = next_RRN;
			break;
		}
	}

	//if no key was found exit out
	if(next_RRN == -1){
		return false;
	}

	index_fstream.close();

	std::fstream DB_fstream(global::MESSAGE_DIGEST_DB.c_str());

	while(true){

		DB_fstream.seekp((next_RRN * global::RECORD_SIZE) + global::MESSAGE_DIGEST_SIZE, std::ios::beg);

		//read the RRN (pointer to next in list)
		char RRN_ch[global::MESSAGE_DIGEST_SIZE + 1];
		DB_fstream.get(RRN_ch, global::RRN_SIZE + 1, ' ');
		temp.assign(RRN_ch, DB_fstream.gcount());

		if(temp == global::RECORD_LIST_END){

			DB_fstream.seekp((next_RRN * global::RECORD_SIZE) + global::MESSAGE_DIGEST_SIZE, std::ios::beg);

			if(RRN_FreeStart != -1){
				//link the RRN_Free list with the end of the deleted record list
				std::ostringstream RRN_FreeStartLink_s;
				RRN_FreeStartLink_s << RRN_FreeStart;
				std::string RRN_FreeStartLink = RRN_FreeStartLink_s.str();
				RRN_FreeStartLink.append(global::RRN_SIZE - RRN_FreeStartLink.size(), ' ');
				DB_fstream << RRN_FreeStartLink;
			}
			else{
				std::string RRN_FreeStartLink = global::RECORD_LIST_END;
				RRN_FreeStartLink.append(global::RRN_SIZE - RRN_FreeStartLink.size(), ' ');
				DB_fstream << RRN_FreeStartLink;
			}

			break;
		}
		else{
			//set next_RRN to visit on next iteration
			next_RRN = atoi(temp.c_str());
		}
	}

	//update the index
	update_RRN_Free(index_fstream);
}

bool digest_DB::retrieveDigests(std::string key, std::deque<std::string> & digests)
{
	std::fstream index_fstream(global::MESSAGE_DIGEST_INDEX.c_str());

	std::string temp;  //holds lines from DB or index
	int next_RRN = -1; //holds next RRN to be visited

	//not interested in the first line(RRN_Free, RRN_Count);
	index_fstream.seekg(global::RRN_SIZE * 2);

	//find the RRN of the start of the digests list
	while(getline(index_fstream, temp)){

		if(key == temp.substr(0, global::MESSAGE_DIGEST_SIZE)){

			//store the RRN of the start of the list
			next_RRN = atoi(temp.substr(global::MESSAGE_DIGEST_SIZE, global::RECORD_SIZE - temp.find_first_of(' ', global::MESSAGE_DIGEST_SIZE)).c_str());
			break;
		}
	}

	//if no key was found exit out
	if(next_RRN == -1){
		return false;
	}

	std::fstream DB_fstream(global::MESSAGE_DIGEST_DB.c_str());

	//read the list and copy to digests deque
	while(next_RRN != -1){

		DB_fstream.seekp(next_RRN * global::RECORD_SIZE, std::ios::beg);

		//read the messageDigest and store it in the digests deque
		char messageDigest_ch[global::MESSAGE_DIGEST_SIZE + 1];
		DB_fstream.get(messageDigest_ch, global::MESSAGE_DIGEST_SIZE +1);
		temp.assign(messageDigest_ch, DB_fstream.gcount());
		digests.push_front(temp);

		//read the RRN (pointer to next in list)
		char RRN_ch[global::MESSAGE_DIGEST_SIZE + 1];
		DB_fstream.get(RRN_ch, global::RRN_SIZE + 1, ' ');
		temp.assign(RRN_ch, DB_fstream.gcount());

		if(temp == global::RECORD_LIST_END){
			next_RRN = -1;
		}
		else{
			next_RRN = atoi(temp.c_str());
		}
	}

	return true;
}

void digest_DB::update_RRN_Free(std::fstream & index_fstream)
{
	index_fstream.seekp(0, std::ios::beg);

	std::ostringstream RRN_FreeIndex_s;
	RRN_FreeIndex_s << RRN_Free;
	std::string RRN_FreeIndex = RRN_FreeIndex_s.str();
	RRN_FreeIndex.append(global::RRN_SIZE - RRN_FreeIndex.size(), ' ');
	index_fstream << RRN_FreeIndex;
}

void digest_DB::update_RRN_Count(std::fstream & index_fstream)
{
	index_fstream.seekp(global::RRN_SIZE, std::ios::beg);

	std::ostringstream RRN_CountIndex_s;
	RRN_CountIndex_s << RRN_Count;
	std::string RRN_CountIndex = RRN_CountIndex_s.str();
	RRN_CountIndex.append(global::RRN_SIZE - RRN_CountIndex.size(), ' ');
	index_fstream << RRN_CountIndex;
}

