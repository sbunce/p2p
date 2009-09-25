#ifndef H_FILE
#define H_FILE

//custom
#include "block_request.hpp"
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

	const std::string hash;
	const std::string path;
	const boost::uint64_t file_size;


	//block_request Block_Request;
private:

};
#endif
