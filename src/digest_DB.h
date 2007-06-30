#ifndef H_DIGEST_DB
#define H_DIGEST_DB

#include <vector>
#include <string>

const int MESSAGE_DIGEST_SIZE = 40;
const int RRN_SIZE = 7;
const std::string RECORD_DELIMITER = "\n";
const int RECORD_SIZE = MESSAGE_DIGEST_SIZE + RRN_SIZE + RECORD_DELIMITER.size();
const std::string MESSAGE_DIGEST_INDEX = "messageDigest.idx";
const std::string MESSAGE_DIGEST_DB = "messageDigest.db";


/*
MESSAGE_DIGEST_INDEX file format:
The first line is a RRN for the start of the free space chain.
The other lines are as follows:
<key>|<RRN>
where key is the hash of the hashes of all the superBlocks in the file

MESSAGE_DIGEST_DB file format:
<messageDigest>|<RRN>
the record size is fixed length at MESSAGE_DIGEST_SIZE + RRN_SIZE + DELIMITER.size()
*/

class digest_DB
{
public:
	digest_DB();
	/*
	addDigests      - add digests for a file to the database
	                - returns false if digests already exist for specified key
	retrieveDigests - retrieves digests associated with a key
	                - returns false if key wasn't found
	*/
	bool addDigests(std::string key, std::vector<std::string> & digests);
	bool retrieveDigests(std::string key, std::vector<std::string> & digests);

private:

};
#endif

