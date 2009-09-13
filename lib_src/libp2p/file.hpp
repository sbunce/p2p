#ifndef H_FILE
#define H_FILE

//custom
#include "block_request.hpp"
#include "protocol.hpp"
#include "shared_files.hpp"

//include
#include <boost/cstdint.hpp>
#include <boost/utility.hpp>

//standard
#include <string>

class file : private boost::noncopyable
{
public:
	file(const shared_files::file & File);

	const std::string hash;
	const boost::uint64_t file_size;

	block_request Block_Request;

private:

};
#endif
