#ifndef H_FILE
#define H_FILE

//custom
#include "block_request.hpp"
#include "protocol.hpp"

//include
#include <boost/cstdint.hpp>
#include <boost/utility.hpp>

//standard
#include <string>

class file : private boost::noncopyable
{
public:
	file(
		const std::string & hash_in,
		const boost::uint64_t & file_size_in
	);

	const std::string hash;
	const boost::uint64_t file_size;

	block_request Block_Request;

private:

};
#endif
