#include <fstream>
#include <iostream>
#include <sstream>

#include "digest_DB.h"

digest_DB::digest_DB()
{
	//check if the index file exists, if it doesn't then create it
	std::ifstream fin(MESSAGE_DIGEST_INDEX.c_str());

	if(fin.is_open()){
		fin.close();
	}
	else{
		std::ofstream fout(MESSAGE_DIGEST_INDEX.c_str());

		//set RRN_Free chain to -1 to indicate no available records for overwrite
		std::string RRN_Free = "-1";
		fout << RRN_Free;

		fout.close();
	}

	//check if the database file exists, if it doesn't then create it
	fin.open(MESSAGE_DIGEST_DB.c_str());

	if(fin.is_open()){
		fin.close();
	}
	else{
		std::ofstream fout(MESSAGE_DIGEST_DB.c_str());
		fout.close();
	}
}

bool digest_DB::addDigests(std::string key, std::vector<std::string> & digests)
{
	std::ifstream index_fin(MESSAGE_DIGEST_INDEX.c_str());

	if(index_fin.is_open()){
		std::string temp;
		std::string RRN_Free; //pointer to start of available record chain

		//get the start of the avail list chain for free space
		getline(index_fin, RRN_Free);

		//check to see if this set of digests already exists
		while(getline(index_fin, temp)){

			if(key == temp.substr(0, MESSAGE_DIGEST_SIZE)){
				return false;
			}
		}

		index_fin.close();

		std::ifstream DB_fin(MESSAGE_DIGEST_DB.c_str());
		std::ofstream DB_fout(MESSAGE_DIGEST_DB.c_str());

		if(DB_fin.is_open() && DB_fout.is_open()){

			std::string previous_RRN = "-1";

			for(std::vector<std::string>::iterator iter0 = digests.begin(); iter0 != digests.end(); iter0++){
std::cout << "*iter0: " << *iter0 << "\n";
std::cout << "RRN_Free: \"" << RRN_Free << "\"\n";
				if(RRN_Free == "-1"){
std::cout << "POIK\n";
					previous_RRN.append(RRN_SIZE - previous_RRN.size(), ' ');
					std::string record = *iter0 + previous_RRN;
					previous_RRN = DB_fout.tellp();
std::cout << "previous_RRN: " << previous_RRN << "\n";
					DB_fout.write(record.c_str(), record.size());
				}
				else{ //write to available record space

				}
			}
		}
	}
	else{
#ifdef DEBUG
		std::cout << "error: digest_DB::addDigests() couldn't open " << MESSAGE_DIGEST_INDEX << " or " << MESSAGE_DIGEST_DB << "\n";
#endif
	}
}

bool digest_DB::retrieveDigests(std::string key, std::vector<std::string> & digests)
{
	std::ifstream fin(MESSAGE_DIGEST_INDEX.c_str());

	if(fin.is_open()){
		std::string temp;
		std::string RRN;

		//check to see if this set of digests already exists
		while(getline(fin, temp)){

			if(key == temp.substr(0, MESSAGE_DIGEST_SIZE)){

				RRN = temp.substr(MESSAGE_DIGEST_SIZE, RRN_SIZE);

				//trim off trailing space if there is any
				for(std::string::iterator iter0 = RRN.begin(); iter0 != RRN.end(); iter0++){
					if(*iter0 == ' '){
						RRN.erase(iter0++); //increment iterator while still valid
					}
				}
			}
		}

		if(RRN.size() != 0){

		}
		else{

		}
	}
	else{
#ifdef DEBUG
		std::cout << "error: digest_DB::retrieveDigests() couldn't open " << MESSAGE_DIGEST_INDEX << "\n";
#endif
	}
}

