#ifndef H_FILE
#define H_FILE

//custom
#include "block_request.hpp"
#include "database.hpp"
#include "file_info.hpp"
#include "protocol.hpp"

//include
#include <boost/cstdint.hpp>
#include <boost/utility.hpp>

//standard
#include <string>

class file : private boost::noncopyable
{
public:
	file(const file_info & FI);

	const std::string path;                //full path to downloading file
	const boost::uint64_t file_size;       //complete size of downloading file
	const boost::uint64_t block_count;     //number of file blocks
	const boost::uint64_t last_block_size; //size of last block in file

	/*
	block_size:
		Returns size of the specified file block.
		Precondition: block_num < block_count
	complete:
		Returns true if the file is complete.
	*/
	unsigned block_size(const boost::uint64_t block_num);
	bool complete();

	//used for requesting blocks, also keeps track of all blocks we have
	block_request Block_Request;

private:

};
#endif
