/*
************************** Open Database: **************************************
 database::connection DB("database");

************************** Query: **********************************************
No call back:
 DB.query("SELECT * FROM table");

With function call back:
 DB.query("SELECT * FROM table", call_back);               //non-member function
 DB.query("SELECT * FROM table", &Test, &test::call_back); //member function

With function call back with object:
 test Test;
 std::string str = "NARF!";
 DB.query("SELECT * FROM table", call_back_with_object, &str);
 DB.query("SELECT * FROM table", &Test, &test::call_back_with_object, &str);

************************** Example Call Back Functions: ************************
Regular call back:
	int call_back(int columns, char ** response, char ** column_name);

Call back with object:
	int call_back(std::string str, int columns, char ** response, char ** column_name);

************************** Blobs: **********************************************
Create table with blob field:
	DB.query("CREATE TABLE test(test_blob BLOB)");

Allocate blob of a specified size:
	boost::int64_t rowid = DB.blob_allocate("INSERT INTO test(test_blob) VALUES(?)", 1);

Resizing blob is not possible, but you can replace a blob:
	boost::int64_t rowid = DB.blob_allocate("UPDATE test SET test_blob = ?", 2);

Open a blob (rowid is required, hope you didn't lose it!'):
	database::blob Blob("test", "test_blob", rowid);

Write blob:
	const char * write_buff = "ABC";
	DB.blob_write(Blob, write_buff, 4, 0);

Read blob:
	char read_buff[4];
	DB.blob_read(Blob, read_buff, 4, 0);
*/
#ifndef H_DATABASE_CONNECTION
#define H_DATABASE_CONNECTION

//include
#include <atomic_int.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <logger.hpp>
#include <sqlite3.h>

namespace database{
//object needed for reading/writing a blob
class blob
{
public:
	//a read or write to a blob constructed with this will fail
	blob():
		rowid(0) //indicates blob is invalid
	{}

	blob(
		const std::string & table_in,
		const std::string & column_in,
		const boost::int64_t rowid_in
	):
		table(table_in),
		column(column_in),
		rowid(rowid_in)
	{}

	blob(const blob & Blob)
	{
		*this = Blob;
	}

	std::string table;
	std::string column;

	//primary key of row (starts at 1)
	boost::int64_t rowid;
};

class connection : private boost::noncopyable
{
public:
	connection(
		const std::string & path_in
	):
		path(path_in),
		connected(false)
	{}

	~connection()
	{
		if(connected){
			if(sqlite3_close(DB_handle) != SQLITE_OK){
				LOGGER << "sqlite error: " << sqlite3_errmsg(DB_handle);
			}
		}
	}

	/*
	Pre-allocate space for blob so that incremental read/write may happen. Return
	false if allocation failed.
	*/
	bool blob_allocate(const std::string & query, const int size)
	{
		boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
		connect();
		while(true){
			/*
			Go back to the beginning of the process whenever a SQLITE_BUSY is
			received. Only return false if there is an error. It proved to be
			problematic (acted like infinite loop) when we tried to roll back the
			sqlite3_step() only and retry.
			*/
			int ret;
			sqlite3_stmt * prepared_statement;
			ret = sqlite3_prepare_v2(
				DB_handle,
				query.c_str(),
				query.size(),
				&prepared_statement,
				0 //not needed unless multiple statements
			);
			if(ret == SQLITE_BUSY){
				if(sqlite3_finalize(prepared_statement) != SQLITE_OK){
					LOGGER << sqlite3_errmsg(DB_handle);
				}
				boost::this_thread::yield();
				continue;
			}else if(ret != SQLITE_OK){
				if(sqlite3_finalize(prepared_statement) != SQLITE_OK){
					LOGGER << sqlite3_errmsg(DB_handle);
				}
				return false;
			}

			ret = sqlite3_bind_zeroblob(prepared_statement, 1, size);
			if(ret == SQLITE_BUSY){
				if(sqlite3_finalize(prepared_statement) != SQLITE_OK){
					LOGGER << sqlite3_errmsg(DB_handle);
				}
				boost::this_thread::yield();
				continue;
			}else if(ret != SQLITE_OK){
				if(sqlite3_finalize(prepared_statement) != SQLITE_OK){
					LOGGER << sqlite3_errmsg(DB_handle);
				}
				return false;
			}

			ret = sqlite3_step(prepared_statement);
			if(ret == SQLITE_DONE){
				return true;
			}else if(ret == SQLITE_BUSY){
				if(sqlite3_finalize(prepared_statement) != SQLITE_OK){
					LOGGER << sqlite3_errmsg(DB_handle);
				}
				boost::this_thread::yield();
				continue;
			}else{
				return false;
			}
		}
	}

	/*
	Read data from blob. Returns true if success else false.
	buff:
		Pointer to memory where bytes can be stored. This must be at least as big
		as size.
	size:
		Size of data to read in to buff.
	offset:
		Offset (bytes) in to the blob to start reading.
	*/
	bool blob_read(const blob & Blob, char * const buff, const int size, const int offset)
	{
		boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
		connect();
		sqlite3_blob * blob_handle;
		if(!blob_open(Blob, false, blob_handle)){
			return false;
		}
		if(sqlite3_blob_read(blob_handle, static_cast<void *>(buff), size, offset) != SQLITE_OK){
			LOGGER << sqlite3_errmsg(DB_handle);
			return false;
		}
		return blob_close(blob_handle);
	}

	/*
	Write data to blob. Returns true if success else false.
	See documentation for blob_write to find out what the parameters do.
	*/
	bool blob_write(const blob & Blob, const char * const buff, const int size, const int offset)
	{
		boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
		connect();
		sqlite3_blob * blob_handle;
		if(!blob_open(Blob, true, blob_handle)){
			return false;
		}
		if(sqlite3_blob_write(blob_handle, static_cast<const void *>(buff), size,
			offset) != SQLITE_OK)
		{
			LOGGER << sqlite3_errmsg(DB_handle);
			return false;
		}
		return blob_close(blob_handle);
	}

	//query with no call back
	int query(const std::string & query)
	{
		boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
		connect();
		int code;
		while((code = sqlite3_exec(DB_handle, query.c_str(), NULL, NULL, NULL)) != SQLITE_OK){
			if(code == SQLITE_BUSY){
				boost::this_thread::yield();
			}else{
				LOGGER << "sqlite error " << code << ": " << sqlite3_errmsg(DB_handle)
					<< " query: " << query;
				exit(1);
			}
		}
		return code;
	}

	//query with function call back
	int query(const std::string & query, int (*fun_ptr)(int, char **, char **))
	{
		boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
		connect();
		int code;
		while((code = sqlite3_exec(DB_handle, query.c_str(), fun_call_back_wrapper,
			(void *)fun_ptr, NULL)) != SQLITE_OK)
		{
			if(code == SQLITE_BUSY){
				boost::this_thread::yield();
			}else{
				LOGGER << "sqlite error " << code << ": " << sqlite3_errmsg(DB_handle)
					<< " query: " << query;
				exit(1);
			}
		}
		return code;
	}

	//query with function call back and object
	template <typename T>
	int query(const std::string & query, int (*fun_ptr)(T, int, char **, char **), T t)
	{
		boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
		connect();
		//std::pair<func signature, object>
		std::pair<int (*)(T, int, char **, char **), T*> call_back_info(fun_ptr, &t);
		int code;
		while((code = sqlite3_exec(DB_handle, query.c_str(), fun_with_object_call_back_wrapper<T>,
			static_cast<void *>(&call_back_info), NULL)) != SQLITE_OK)
		{
			if(code == SQLITE_BUSY){
				boost::this_thread::yield();
			}else{
				LOGGER << "sqlite error " << code << ": " << sqlite3_errmsg(DB_handle)
					<< " query: " << query;
				exit(1);
			}
		}
		return code;
	}

	//query with member function call back
	template <typename T>
	int query(const std::string & query, T * t, int (T::*memfun_ptr)(int, char **, char **))
	{
		boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
		connect();
		//std::pair<object with func, func signature>
		std::pair<T*, int (T::*)(int, char **, char **)> call_back_info(t, memfun_ptr);
		int code;
		while((code = sqlite3_exec(DB_handle, query.c_str(), memfun_call_back_wrapper<T>,
			static_cast<void *>(&call_back_info), NULL)) != SQLITE_OK)
		{
			if(code == SQLITE_BUSY){
				boost::this_thread::yield();
			}else{
				LOGGER << "sqlite error " << code << ": " << sqlite3_errmsg(DB_handle)
					<< " query: " << query;
				exit(1);
			}
		}
		return code;
	}

	//query with member function call back and object
	template <typename T, typename T_obj>
	int query(const std::string & query, T * t, int (T::*memfun_ptr)(T_obj, int, char **, char **),
		T_obj T_Obj)
	{
		boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
		connect();
		std::pair<std::pair<T*, int (T::*)(T_obj, int, char **, char **)>, T_obj*>
			info(std::make_pair(t, memfun_ptr), &T_Obj);
		int code;
		while((code = sqlite3_exec(DB_handle, query.c_str(),
			memfun_with_object_call_back_wrapper<T, T_obj>,
			static_cast<void *>(&info), NULL)) != SQLITE_OK)
		{
			if(code == SQLITE_BUSY){
				boost::this_thread::yield();
			}else{
				LOGGER << "sqlite error " << code << ": " << sqlite3_errmsg(DB_handle)
					<< " query: " << query;
				exit(1);
			}
		}
		return code;
	}

private:
	/*
	Recursive_Mutex locks access to all data members and insures that only one
	database operation at a time happens.
	*/
	boost::recursive_mutex Recursive_Mutex;
	sqlite3 * DB_handle;
	bool connected;      //used for lazy connect
	std::string path;    //used for lazy connect

	/*
	The connection object is lazy. It only connects to the database when a query
	is done. This function connects to the database.
	*/
	void connect()
	{
		boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
		if(!connected){
			if(sqlite3_open_v2(path.c_str(), &DB_handle,
				SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX,
				0) != SQLITE_OK)
			{
				LOGGER << "sqlite error: " << sqlite3_errmsg(DB_handle);
				exit(1);
			}

			//timeout in ms, this should not be zero with concurrency
			if(sqlite3_busy_timeout(DB_handle, 4000) != SQLITE_OK){
				LOGGER << "sqlite error: " << sqlite3_errmsg(DB_handle);
				exit(1);
			}

			//move free pages to EOF and truncate at every commit
			int code;
			while((code = sqlite3_exec(DB_handle, "PRAGMA auto_vacuum = full;",
				NULL, NULL, NULL)) != SQLITE_OK)
			{
				if(code == SQLITE_BUSY){
					boost::this_thread::yield();
				}else{
					LOGGER << "sqlite error " << code << ": " << sqlite3_errmsg(DB_handle);
					exit(1);
				}
			}
			connected = true;

			//this is no longer used, save space by clearing it
			path.clear();
		}
	}

	/* Call Back Wrappers
	Wrappers are needed because a function with a C-signature is needed for
	SQLite to do a call back. A static function has a C-signature.
	*/
	//function call back
	static int fun_call_back_wrapper(void * ptr, int columns, char ** response,
		char ** column_name)
	{
		int (*fun_ptr)(int, char **, char **)
			= reinterpret_cast<int (*)(int, char **, char **)>(ptr);
		return fun_ptr(columns, response, column_name);
	}

	//function call back with object passed to call back
	template <typename T>
	static int fun_with_object_call_back_wrapper(void * ptr, int columns,
		char ** response, char ** column_name)
	{
		std::pair<int (*)(T, int, char **, char **), T*> *
			info = reinterpret_cast<std::pair<int (*)(T, int, char **, char **), T*> *>(ptr);
		int (*fun_ptr)(T, int, char **, char **) = info->first;
		T * obj = info->second; //object to pass to function
		return fun_ptr(*obj, columns, response, column_name);
	}

	//member function call back
	template <typename T>
	static int memfun_call_back_wrapper(void * ptr, int columns, char ** response,
		char ** column_name)
	{
		std::pair<T*, int (T::*)(int, char **, char **)> *
			info = reinterpret_cast<std::pair<T*, int (T::*)(int, char **, char **)> *>(ptr);
		T * obj = info->first; //object which has member function to call
		int (T::*mem_fun_ptr)(int, char **, char **) = info->second;
		return (obj->*mem_fun_ptr)(columns, response, column_name);
	}

	//member function call back with object passed to call back
	template <typename T, typename T_obj>
	static int memfun_with_object_call_back_wrapper(void * ptr, int columns,
		char ** response, char ** column_name)
	{
		std::pair<std::pair<T*, int (T::*)(T_obj, int, char **, char **)>, T_obj*> *
			info = reinterpret_cast<std::pair<std::pair<T*,
			int (T::*)(T_obj, int, char **, char **)>, T_obj*> *>(ptr);
		T * obj = info->first.first; //object which has member function to call
		int (T::*mem_fun_ptr)(T_obj, int, char **, char **) = info->first.second;
		T_obj * t_obj = info->second; //object to pass to member function
		return (obj->*mem_fun_ptr)(*t_obj, columns, response, column_name);
	}

	//close_blob
	bool blob_close(sqlite3_blob * blob_handle)
	{
		boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
		if(sqlite3_blob_close(blob_handle) == SQLITE_OK){
			return true;
		}else{
			LOGGER << "sqlite error: " << sqlite3_errmsg(DB_handle);
			return false;
		}
	}

	//open blob
	bool blob_open(const blob & Blob, const bool & writeable, sqlite3_blob *& blob_handle)
	{
		boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
		assert(!Blob.column.empty());
		assert(!Blob.table.empty());
		assert(Blob.rowid > 0);
		int code;
		while((code = sqlite3_blob_open(
			DB_handle,
			"main",               //DB name ("main" is default)
			Blob.table.c_str(),
			Blob.column.c_str(),
			Blob.rowid,           //row ID (primary key)
			reinterpret_cast<const int &>(writeable), //0 = read only, non-zero = read/write
			&blob_handle
		)) != SQLITE_OK){
			if(code == SQLITE_BUSY){
				boost::this_thread::yield();
			}else{
				LOGGER << "sqlite error " << code << ": " << sqlite3_errmsg(DB_handle);
				return false;
			}
		}
		return true;
	}
};
}//end of database namespace
#endif
