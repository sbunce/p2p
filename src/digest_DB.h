#ifndef H_DIGEST_DB
#define H_DIGEST_DB

#include <deque>
#include <string>

#include "globals.h"

/*
MESSAGE_DIGEST_INDEX file format:
First Line: <index_RecordCount><index_RecordFree>
All Other Lines: <key><RRN>

MESSAGE_DIGEST_DB file format:
First Line: <DB_RecordCount><DB_RecordFree>
All Other Lines: <key><RRN>

All records have a \n after them for readability. This is not necessary and can
be removed by getting rid of the end of read_RRN() and by getting rid of the +1
in the globals.h file.
*/

class digest_DB
{
public:
	digest_DB();
	/*
	addDigests      - add digests for a file to the database
	                - returns false if digests already exist for specified key
	deleteDigests   - removes a set of digests and adds it to the DB_RecordFree list
	                - doesn't actually remove but marks the records as free(can be overwritten)
	                - returns false if nothing deleted
	retrieveDigests - retrieves digests associated with a key
	                - returns false if key wasn't found
	*/
	bool addDigests(std::string key, std::deque<std::string> & digests);
	bool deleteDigests(std::string key);
	bool retrieveDigests(std::string key, std::deque<std::string> & digests);

private:
	int DB_RecordCount;    //how many records in database(including deleted)
	int DB_RecordFree;     //start of the free record list for the database
	int index_RecordCount; //how many records in database(including deleted)
	int index_RecordFree;  //start of the free record list for the index

	/*
	addKeyToIndex            - adds the key for a list to the index file
	locateList               - locates the RRN of the start of a list associated with key
	                         - returns false if list not located

	//The read pointer must be positioned properly before these are used.
	//Both of these return false if EOF was hit.
	readMessageDigest        - reads MESSAGE_DIGEST_SIZE sized string from the index or DB
	read_RRN                 - reads RRN_SIZE sized string from the index or DB


	update_index_RecordFree  - writes index_RecordFree to file
	update_index_RecordCount - writes index_RecordCount to file
	update_DB_RecordFree     - writes DB_RecordFree to file
	update_DB_RecordCount    - writes DB_RecordCount to file
	*/
	void addKeyToIndex(std::string key, int RRN);
	bool locateList(std::string key, std::string & RRN);
	bool readMessageDigest(std::fstream & Index_or_DB, std::string & messageDigest);
	bool read_RRN(std::fstream & Index_or_DB, std::string & RRN);
	void update_DB_RecordCount();
	void update_DB_RecordFree();
	void update_index_RecordCount();
	void update_index_RecordFree();
};
#endif

