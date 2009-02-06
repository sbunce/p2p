//THREADSAFE
#ifndef H_DATABASE_BLOB
#define H_DATABASE_BLOB

//custom
#include "global.hpp"

//std
#include <string>

namespace database{
//object needed for reading/writing a blob
class blob
{
public:
	blob(
		const std::string & table_in,
		const std::string & column_in,
		const boost::int64_t & rowid_in
	):
		table(table_in),
		column(column_in),
		rowid(rowid_in)
	{}

	const std::string table;
	const std::string column;
	const boost::int64_t rowid;
};
}//end database namespace
#endif
