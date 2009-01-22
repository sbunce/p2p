/*
Short:
	This container wraps all the call back and incremental blob reading/writing
	features of SQLite2.

Copying:
	This container may be copied, but when it is the same database connection is
	used for every copy. This container (and copies of this container) are thread
	safe.

Ref Counting:
	This object reference counts. The blob_handle's are closed when they are
	destructed. When all sqlite3_wrapper and blob_handle instantations are
	destructed the database connection is closed.

************************** Open Database: **************************************
 sqlite3_wrapper DB("./database");

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

Call back with object (can be value or reference):
	int call_back(std::string str, int columns, char ** response, char ** column_name);
	int call_back(std::string & str, int columns, char ** response, char ** column_name);

************************** Blobs: **********************************************
Create table with blob field:
	DB.query("CREATE TABLE test(test_blob BLOB)");

Allocate blob of a specified size:
	boost::int64_t rowid = DB.blob_allocate("INSERT INTO test(test_blob) VALUES(?)", 1);

Resizing blob is not possible, but you can replace a blob:
	boost::int64_t rowid = DB.blob_allocate("UPDATE test SET test_blob = ?", 2);

Open a blob (rowid is required, hope you didn't lose it!'):
	sqlite3_wrapper::blob Blob("test", "test_blob", rowid);

Write blob:
	const char * write_buff = "ABC";
	Blob.write(BH, write_buff, 4, 0);

Read blob:
	char read_buff[4];
	Blob.read(BH, read_buff, 4, 0);
*/
#ifndef H_SQLITE3_WRAPPER
#define H_SQLITE3_WRAPPER

//boost
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/shared_ptr.hpp>

//custom
#include "atomic_int.h"
#include "global.h"

//sqlite
#include <sqlite3.h>

//std
#include <exception>
#include <iostream>

namespace sqlite3_wrapper{

class database
{
	friend class blob;
public:
	database():
		ref_cnt(new atomic_int<int>(0)),
		Mutex(new boost::recursive_mutex())
	{
		if(sqlite3_open_v2(global::DATABASE_PATH.c_str(), &DB_handle,
			SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX,
			0) != SQLITE_OK)
		{
			LOGGER << "sqlite error: " << sqlite3_errmsg(DB_handle);
			throw SWE;
		}
		if(sqlite3_busy_timeout(DB_handle, global::DB_TIMEOUT) != SQLITE_OK){
			LOGGER << "sqlite error: " << sqlite3_errmsg(DB_handle);
			throw SWE;
		}
		if(sqlite3_exec(DB_handle,
			"PRAGMA auto_vacuum = full;"     //move free pages to EOF and truncate at every commit
			"PRAGMA journal_mode = PERSIST;" //do not delete the journal file between uses
			"PRAGMA cache_size = 32768;"     //increase max in memory pages from 2000
			"PRAGMA temp_store = MEMORY;"    //make sure temp store is memory and not a file
			, NULL, NULL, NULL) != SQLITE_OK){
			LOGGER << "sqlite error: " << sqlite3_errmsg(DB_handle);
		}
	}

	database(
		const sqlite3_wrapper::database & DB
	):
		DB_handle(DB.DB_handle),
		ref_cnt(DB.ref_cnt)
	{
		++*ref_cnt;
	}

	~database()
	{
		if(*ref_cnt == 0){
			if(sqlite3_close(DB_handle) != SQLITE_OK){
				LOGGER << "sqlite error: " << sqlite3_errmsg(DB_handle);
			}
		}else{
			--*ref_cnt;
		}
	}

	//allocate blob of specified size
	boost::int64_t blob_allocate(const std::string & query, const int & size)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		sqlite3_stmt * prepared_statement;
		if(sqlite3_prepare_v2(
			DB_handle,
			query.c_str(),
			query.size(),
			&prepared_statement,
			0 //not needed unless multiple statements
		) != SQLITE_OK){
			LOGGER << sqlite3_errmsg(DB_handle);
		}
		if(sqlite3_bind_zeroblob(prepared_statement, 1, size) != SQLITE_OK){
			LOGGER << sqlite3_errmsg(DB_handle);
			exit(1);
		}
		int code;
		while((code = sqlite3_step(prepared_statement)) != SQLITE_DONE){
			LOGGER << "sqlite error " << code << ": " << sqlite3_errmsg(DB_handle);
		}
		if(sqlite3_finalize(prepared_statement) != SQLITE_OK){
			LOGGER << sqlite3_errmsg(DB_handle);
			exit(1);
		}
		return sqlite3_last_insert_rowid(DB_handle);
	}

	//query with no call back
	int query(const std::string & query)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		int code;
		if((code = sqlite3_exec(DB_handle, query.c_str(), NULL, NULL, NULL)) != SQLITE_OK){
			LOGGER << "sqlite error " << code << ": " << sqlite3_errmsg(DB_handle) << " query: " << query;
		}
		return code;
	}

	//query with function call back
	int query(const std::string & query, int (*fun_ptr)(int, char **, char **))
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		int code;
		if((code = sqlite3_exec(DB_handle, query.c_str(), fun_call_back_wrapper, (void *)fun_ptr, NULL)) != 0){
			LOGGER << "sqlite error " << code << ": " << sqlite3_errmsg(DB_handle) << " query: " << query;
		}
		return code;
	}

	//query with function call back and object
	template <typename T>
	int query(const std::string & query, int (*fun_ptr)(T, int, char **, char **), T t)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		//std::pair<func signature, object>
		std::pair<int (*)(T, int, char **, char **), T*> call_back_info(fun_ptr, &t);
		int code;
		if((code = sqlite3_exec(DB_handle, query.c_str(), fun_with_object_call_back_wrapper<T>,
			(void *)&call_back_info, NULL)) != SQLITE_OK)
		{
			LOGGER << "sqlite error " << code << ": " << sqlite3_errmsg(DB_handle) << " query: " << query;
		}
		return code;
	}

	//query with function call back and object by reference
	template <typename T>
	int query(const std::string & query, int (*fun_ptr)(T&, int, char **, char **), T & t)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		//std::pair<func signature, object>
		std::pair<int (*)(T&, int, char **, char **), T*> call_back_info(fun_ptr, &t);
		int code;
		if((code = sqlite3_exec(DB_handle, query.c_str(), fun_with_object_reference_call_back_wrapper<T>,
			(void *)&call_back_info, NULL)) != SQLITE_OK)
		{
			LOGGER << "sqlite error " << code << ": " << sqlite3_errmsg(DB_handle) << " query: " << query;
		}
		return code;
	}

	//query with member function call back
	template <typename T>
	int query(const std::string & query, T * t, int (T::*memfun_ptr)(int, char **, char **))
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		//std::pair<object with func, func signature>
		std::pair<T*, int (T::*)(int, char **, char **)> call_back_info(t, memfun_ptr);
		int code;
		if((code = sqlite3_exec(DB_handle, query.c_str(), memfun_call_back_wrapper<T>,
			(void *)&call_back_info, NULL)) != SQLITE_OK)
		{
			LOGGER << "sqlite error " << code << ": " << sqlite3_errmsg(DB_handle) << " query: " << query;
		}
		return code;
	}

	//query with member function call back and object
	template <typename T, typename T_obj>
	int query(const std::string & query, T * t, int (T::*memfun_ptr)(T_obj, int, char **, char **), T_obj T_Obj)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		/*
		std::pair<std::pair<object with func, func signature>, object> >
		A boost::tuple would be nice here but I want to leave this stdlib only.
		*/
		std::pair<std::pair<T*, int (T::*)(T_obj, int, char **, char **)>, T_obj*>
			call_back_info(std::make_pair(t, memfun_ptr), &T_Obj);
		int code;
		if((code = sqlite3_exec(DB_handle, query.c_str(), memfun_with_object_call_back_wrapper<T, T_obj>,
			(void *)&call_back_info, NULL)) != SQLITE_OK)
		{
			LOGGER << "sqlite error " << code << ": " << sqlite3_errmsg(DB_handle) << " query: " << query;
		}
		return code;
	}

	//query with member function call back and object by reference
	template <typename T, typename T_obj>
	int query(const std::string & query, T * t, int (T::*memfun_ptr)(T_obj&, int, char **, char **), T_obj & T_Obj)
	{
		boost::recursive_mutex::scoped_lock lock(*Mutex);
		/*
		std::pair<std::pair<object with func, func signature>, object> >
		A boost::tuple would be nice here but I want to leave this stdlib only.
		*/
		std::pair<std::pair<T*, int (T::*)(T_obj&, int, char **, char **)>, T_obj*>
			call_back_info(std::make_pair(t, memfun_ptr), &T_Obj);
		int code;
		if((code = sqlite3_exec(DB_handle, query.c_str(), memfun_with_object_reference_call_back_wrapper<T, T_obj>,
			(void *)&call_back_info, NULL)) != SQLITE_OK)
		{
			LOGGER << "sqlite error " << code << ": " << sqlite3_errmsg(DB_handle) << " query: " << query;
		}
		return code;
	}

private:
	sqlite3 * DB_handle;
	boost::shared_ptr<atomic_int<int> > ref_cnt;

	/*
	Mutex for all public functions. A recursive mutex is used because a thread
	might want to do a query inside a call back.
	*/
	boost::shared_ptr<boost::recursive_mutex> Mutex;

	//to be thrown when there are ctor errors
	class sqlite3_wrapper_exception: public std::exception
	{
		virtual const char * what() const throw()
		{
			return "failed to open sqlite database";
		}
	} SWE;

	//call back wrapper for functions
	static int fun_call_back_wrapper(void * obj_ptr, int columns, char ** response, char ** column_name)
	{
		int (*fun_ptr)(int, char **, char **) = (int (*)(int, char **, char **))obj_ptr;
		return fun_ptr(columns, response, column_name);
	}

	//call back wrapper for functions with an object
	template <typename T>
	static int fun_with_object_call_back_wrapper(void * obj_ptr, int columns, char ** response, char ** column_name)
	{
		std::pair<int (*)(T, int, char **, char **), T*> * call_back_info;
		call_back_info = (std::pair<int (*)(T, int, char **, char **), T*> *)obj_ptr;
		int (*fun_ptr)(T, int, char **, char **) = (int (*)(T, int, char **, char **))call_back_info->first;
		return fun_ptr(*(call_back_info->second), columns, response, column_name);
	}

	//call back wrapper for functions with an object by reference
	template <typename T>
	static int fun_with_object_reference_call_back_wrapper(void * obj_ptr, int columns, char ** response, char ** column_name)
	{
		std::pair<int (*)(T&, int, char **, char **), T*> * call_back_info;
		call_back_info = (std::pair<int (*)(T&, int, char **, char **), T*> *)obj_ptr;
		int (*fun_ptr)(T&, int, char **, char **) = (int (*)(T&, int, char **, char **))call_back_info->first;
		return fun_ptr(*(call_back_info->second), columns, response, column_name);
	}

	//call back wrapper for member functions
	template <typename T>
	static int memfun_call_back_wrapper(void * obj_ptr, int columns, char ** response, char ** column_name)
	{
		std::pair<T*, int (T::*)(int, char **, char **)> * call_back_info;
		call_back_info = (std::pair<T*, int (T::*)(int, char **, char **)> *)obj_ptr;
		return ((*call_back_info->first).*(call_back_info->second))(columns, response, column_name);
	}

	//call back wrapper for member functions with an object
	template <typename T, typename T_obj>
	static int memfun_with_object_call_back_wrapper(void * obj_ptr, int columns, char ** response, char ** column_name)
	{
		std::pair<std::pair<T*, int (T::*)(T_obj, int, char **, char **)>, T_obj*> * call_back_info;
		call_back_info = (std::pair<std::pair<T*, int (T::*)(T_obj, int, char **, char **)>, T_obj*> *)obj_ptr;
		return ((*call_back_info->first.first).*(call_back_info->first.second))
			(*(call_back_info->second), columns, response, column_name);
	}

	//call back wrapper for member functions with an object by reference
	template <typename T, typename T_obj>
	static int memfun_with_object_reference_call_back_wrapper(void * obj_ptr, int columns, char ** response, char ** column_name)
	{
		std::pair<std::pair<T*, int (T::*)(T_obj&, int, char **, char **)>, T_obj*> * call_back_info;
		call_back_info = (std::pair<std::pair<T*, int (T::*)(T_obj&, int, char **, char **)>, T_obj*> *)obj_ptr;
		return ((*call_back_info->first.first).*(call_back_info->first.second))
			(*(call_back_info->second), columns, response, column_name);
	}
};

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
		rowid(rowid_in),
		ref_cnt(new atomic_int<int>(0)),
		Mutex(new boost::mutex())
	{}

	void read(char * const buff, const int & size, const int & offset)
	{
		boost::mutex::scoped_lock lock(*Mutex);
		open(false);
		if(sqlite3_blob_read(blob_handle, (void *)buff, size, offset) != SQLITE_OK){
			LOGGER << sqlite3_errmsg(DB.DB_handle);
			exit(1);
		}
		close();
	}

	void write(const char * const buff, const int & size, const int & offset)
	{
		boost::mutex::scoped_lock lock(*Mutex);
		open(true);
		if(sqlite3_blob_write(blob_handle, (const void *)buff, size, offset) != SQLITE_OK){
			LOGGER << sqlite3_errmsg(DB.DB_handle);
			exit(1);
		}
		close();
	}
private:
	void open(const bool & writeable)
	{
		assert(rowid != 0);
		int tries = 0;
		while(sqlite3_blob_open(
			DB.DB_handle,
			"main",         //symbolic DB name ("main" is default)
			table.c_str(),
			column.c_str(),
			rowid,          //row ID (primary key) of row with blob
			(int)writeable, //0 = read only, non-zero = read/write
			&blob_handle
		) != SQLITE_OK){
			if(tries++ >= 7){
				LOGGER << "8 consecutive blob open errors, aborting";
				exit(1);
			}else{
				LOGGER << sqlite3_errmsg(DB.DB_handle);
			}
		}
	}
	void close()
	{
		if(sqlite3_blob_close(blob_handle) != SQLITE_OK){
			LOGGER << "sqlite error: " << sqlite3_errmsg(DB.DB_handle);
		}
	}

	const std::string table;
	const std::string column;
	const boost::int64_t rowid;

	database DB;
	sqlite3_blob * blob_handle;
	boost::shared_ptr<boost::mutex> Mutex; //used for all DB access

	//blob closed if this equal to zero and dtor called
	boost::shared_ptr<atomic_int<int> > ref_cnt;
};

}//end sqlite3_wrapper namespace
#endif
