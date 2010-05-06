#ifndef H_FILE
#define H_FILE

//custom
#include "block_request.hpp"
#include "db_all.hpp"
#include "file_info.hpp"
#include "protocol_tcp.hpp"

//include
#include <boost/cstdint.hpp>
#include <boost/utility.hpp>

//standard
#include <string>

class file : private boost::noncopyable
{
public:
	file(const file_info & FI);

	const std::string path;                 //full path to downloading file
	const boost::uint64_t file_size;        //complete size of downloading file
	const boost::uint64_t file_block_count; //number of file blocks
	const boost::uint64_t last_block_size;  //size of last block in file

	/*
	block_size:
		Returns size of the specified file block.
		Precondition: block_num < block_count
	read_block:
		Reads file block and appends it to buf. Returns true if read succeeded,
		false if read failed.
	write_block:
		Write block to file. Returns true if write succeeded, false if write
		failed.
	*/
	unsigned block_size(const boost::uint64_t block_num);
	bool read_block(const boost::uint64_t block_num, net::buffer & buf);
	bool write_block(const boost::uint64_t block_num, const net::buffer & buf);

	/*
	calc_file_block_count:
		Given the file size, returns the file block count. Used to get
		file_block_count when file not instanted.
	*/
	static boost::uint64_t calc_file_block_count(boost::uint64_t file_size);
};
#endif
