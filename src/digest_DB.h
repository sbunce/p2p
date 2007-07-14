#ifndef H_DIGEST_DB
#define H_DIGEST_DB

#include <deque>
#include <string>

#include "globals.h"

/*
MESSAGE_DIGEST_INDEX file format:
First Line: <RRN_Free><RRN_Count>
All Other Lines: <key><RRN>

MESSAGE_DIGEST_DB file format:
<messageDigest><RRN>
*/

class digest_DB
{
public:
	digest_DB();
	/*
	addDigests      - add digests for a file to the database
	                - returns false if digests already exist for specified key
	deleteDigests   - removes a set of digests and adds it to the RRN_Free list
	                - doesn't actually remove but marks the records as free(can be overwritten)
	                - returns false if nothing deleted
	retrieveDigests - retrieves digests associated with a key
	                - returns false if key wasn't found
	*/
	bool addDigests(std::string key, std::deque<std::string> & digests);
	bool deleteDigests(std::string key);
	bool retrieveDigests(std::string key, std::deque<std::string> & digests);

private:
	int RRN_Free;  //start of the free record list
	int RRN_Count; //how many records there in the database(including deleted)

	/*
	addKeyToIndex    - adds the key to the index file
	update_RRN_Free  - updates the RRN_Free in the index file
	update_RRN_Count - updates the RRN_Count in the index file
	*/
	void addKeyToIndex(std::string key, int RRN);
	void update_RRN_Free();
	void update_RRN_Count();
};
#endif

