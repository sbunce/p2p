#include <sqlite3.h>
#include <db.hpp>

//BEGIN blob
db::blob::blob():
	rowid(0)
{}

db::blob::blob(
	const std::string & table_in,
	const std::string & column_in,
	const boost::int64_t rowid_in
):
	table(table_in),
	column(column_in),
	rowid(rowid_in)
{}

db::blob::blob(const blob & Blob):
	table(Blob.table),
	column(Blob.column),
	rowid(Blob.rowid)
{

}
//END blob

//BEGIN connection
db::connection::connection(
	const std::string & path_in
):
	path(path_in),
	connected(false)
{

}

db::connection::~connection()
{
	if(connected){
		if(sqlite3_close(DB_handle) != SQLITE_OK){
			LOG << sqlite3_errmsg(DB_handle);
		}
	}
}

bool db::connection::blob_allocate(const std::string & query, const int size)
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
			sqlite3_finalize(prepared_statement);
			boost::this_thread::yield();
			continue;
		}else if(ret != SQLITE_OK){
			sqlite3_finalize(prepared_statement);
			return false;
		}

		ret = sqlite3_bind_zeroblob(prepared_statement, 1, size);
		if(ret == SQLITE_BUSY){
			sqlite3_finalize(prepared_statement);
			boost::this_thread::yield();
			continue;
		}else if(ret != SQLITE_OK){
			sqlite3_finalize(prepared_statement);
			return false;
		}

		ret = sqlite3_step(prepared_statement);
		sqlite3_finalize(prepared_statement);
		if(ret == SQLITE_DONE){
			return true;
		}else if(ret == SQLITE_BUSY){
			boost::this_thread::yield();
			continue;
		}else{
			return false;
		}
	}
}

bool db::connection::blob_read(const blob & Blob, char * const buff, const int size, const int offset)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	connect();
	sqlite3_blob * blob_handle;
	if(!blob_open(Blob, false, blob_handle)){
		return false;
	}
	if(sqlite3_blob_read(blob_handle, static_cast<void *>(buff), size, offset) != SQLITE_OK){
		LOG << sqlite3_errmsg(DB_handle);
		return false;
	}
	return blob_close(blob_handle);
}

bool db::connection::blob_size(const blob & Blob, boost::uint64_t & size)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	connect();
	sqlite3_blob * blob_handle;
	if(!blob_open(Blob, false, blob_handle)){
		return false;
	}
	size = sqlite3_blob_bytes(blob_handle);
	return blob_close(blob_handle);
}

bool db::connection::blob_write(const blob & Blob, const char * const buf, const int size, const int offset)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	connect();
	sqlite3_blob * blob_handle;
	if(!blob_open(Blob, true, blob_handle)){
		return false;
	}
	if(sqlite3_blob_write(blob_handle, static_cast<const void *>(buf), size,
		offset) != SQLITE_OK)
	{
		LOG << sqlite3_errmsg(DB_handle);
		return false;
	}
	return blob_close(blob_handle);
}

int db::connection::query(const std::string & query,
	boost::function<int (int, char **, char **)> func)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	connect();
	int code;
	while((code = sqlite3_exec(DB_handle, query.c_str(), call_back_wrapper,
		(void *)&func, NULL)) == SQLITE_BUSY)
	{
		boost::this_thread::yield();
	}
	if(code != SQLITE_OK){
		LOG << sqlite3_errmsg(DB_handle) << ", query \"" << query << "\"";
	}
	return code;
}

void db::connection::connect()
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	if(!connected){
		if(sqlite3_open_v2(path.c_str(), &DB_handle,
			SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX,
			0) != SQLITE_OK)
		{
			LOG << sqlite3_errmsg(DB_handle);
			exit(1);
		}

		//timeout in ms, this should not be zero with concurrency
		if(sqlite3_busy_timeout(DB_handle, 4000) != SQLITE_OK){
			LOG << sqlite3_errmsg(DB_handle);
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
				LOG << code << ": " << sqlite3_errmsg(DB_handle);
				exit(1);
			}
		}
		connected = true;
	}
}

int db::connection::call_back_wrapper(void * ptr, int columns, char ** response,
	char ** column_name)
{
	boost::function<int (int, char **, char **)> &
		func = *reinterpret_cast<boost::function<int (int, char **, char **)> *>(ptr);
	return func(columns, response, column_name);
}

bool db::connection::blob_close(sqlite3_blob * blob_handle)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	if(sqlite3_blob_close(blob_handle) == SQLITE_OK){
		return true;
	}else{
		LOG << sqlite3_errmsg(DB_handle);
		return false;
	}
}

bool db::connection::blob_open(const blob & Blob, const bool writeable, sqlite3_blob *& blob_handle)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	int code;
	while((code = sqlite3_blob_open(
		DB_handle,
		"main",              //DB name ("main" is default)
		Blob.table.c_str(),
		Blob.column.c_str(),
		Blob.rowid,          //row ID (primary key)
		writeable ? 1 : 0,   //0 = read only, non-zero = read/write
		&blob_handle
	)) != SQLITE_OK){
		if(code == SQLITE_BUSY){
			boost::this_thread::yield();
		}else{
			LOG << code << ": " << sqlite3_errmsg(DB_handle);
			return false;
		}
	}
	return true;
}
//END connection

//BEGIN free functions
std::string db::escape(const std::string & str)
{
	char * sqlite3_path = sqlite3_mprintf("%q", str.c_str());
	std::string tmp(sqlite3_path);
	sqlite3_free(sqlite3_path);
	return tmp;
}
//END free functions
