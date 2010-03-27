/*
************************** Open: ***********************************************
 db::connection DB("database");

************************** Query: **********************************************
The call back must have this signature:
	int call_back(int columns, char ** response, char ** column_name);

boost::bind can be used to do call backs to member functions:
	DB.query("SELECT ...", boost::bind(&obj::func, &Obj, _1, _2, _3));

boost::bind can also be used to bind extra parameters to the call back:
	int call_back(int columns, char ** response, char ** column_name
		int & extra);
	DB.query("SELECT ...", boost::bind(&call_back, _1, _2, _3, boost::ref(extra)));
	Note: boost::ref must be used to pass the reference.

************************** Blobs: **********************************************
Create table with blob field:
	DB.query("CREATE TABLE test(blob BLOB)");

Allocate blob of a specified size:
	boost::int64_t rowid = DB.blob_allocate("INSERT INTO test(blob) VALUES(?)", 1);

Resizing blob is not possible, but you can replace a blob:
	boost::int64_t rowid = DB.blob_allocate("UPDATE test SET test_blob = ?", 2);

Open a blob:
	db::blob Blob("test", "test_blob", rowid);

Write blob:
	const char * write_buff = "ABC";
	DB.blob_write(Blob, write_buff, 4, 0);

Read blob:
	char read_buff[4];
	DB.blob_read(Blob, read_buff, 4, 0);
*/
#ifndef H_DB_CONNECTION
#define H_DB_CONNECTION

//include
#include <atomic_int.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <logger.hpp>

namespace db{

//predecl for PIMPL
#ifndef _SQLITE3_H_
class sqlite3;
class sqlite3_blob;
#endif

//object needed for reading/writing a blob
class blob
{
public:
	//construct invalid blob
	blob();

	blob(
		const std::string & table_in,
		const std::string & column_in,
		const boost::int64_t rowid_in
	);

	blob(const blob & Blob);

	std::string table;
	std::string column;
	boost::int64_t rowid; //0 if invalid
};

class connection : private boost::noncopyable
{
public:
	connection(const std::string & path_in);
	~connection();

	/*
	blob_allocate:
		Pre-allocate space for blob so that incremental read/write may happen. Return
		false if allocation failed.
	blob_read:
		Read data from blob. Returns true if success else false.
		buf:    Pointer to space to store read data.
		size:   How many bytes to read.
		offset: Offset in to file from which to start reading.
	blob_size:
		Sets size to size of specified blob. Returns false if failed to open blob.
	blob_write:
		Write data to blob. Returns true if success else false. Refer to blob_read
		documentation to see what paramters do.
	query:
		Execute a query. The func is used to call back with results.
	*/
	bool blob_allocate(const std::string & query, const int size);
	bool blob_read(const blob & Blob, char * const buf, const int size, const int offset);
	bool blob_size(const blob & Blob, boost::uint64_t & size);
	bool blob_write(const blob & Blob, const char * const buf, const int size, const int offset);
	int query(const std::string & query, boost::function<int (int, char **, char **)>
		func = boost::function<int (int, char **, char **)>());

private:
	/*
	Recursive_Mutex locks access to all data members and insures that only one
	database operation at a time happens.
	*/
	boost::recursive_mutex Recursive_Mutex;
	sqlite3 * DB_handle;

	//used for lazy connect
	bool connected;
	std::string path;

	/*
	connect:
		Does lazy connection on first query.
	call_back_wrapper:
		Function with C signature needed to wrap call backs.
	blob_close:
		Close a database blob.
	blob_open:
		Open a database blob.
	*/
	void connect();
	static int call_back_wrapper(void * ptr, int columns, char ** response,
		char ** column_name);
	bool blob_close(sqlite3_blob * blob_handle);
	bool blob_open(const blob & Blob, const bool writeable, sqlite3_blob *& blob_handle);
};

//function to return std::string with control characters escaped
std::string escape(const std::string & str);

}//end of namespace db
#endif
