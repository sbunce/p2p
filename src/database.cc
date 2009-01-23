#include "database.h"

database::database():
	ref_cnt(new atomic_int<int>(0)),
	Mutex(new boost::recursive_mutex())
{
	if(sqlite3_enable_shared_cache(1) != SQLITE_OK){
		LOGGER << "error enabling shared cache\n";
	}
	if(sqlite3_open_v2("DB", &DB_handle,
		SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX,
		0) != SQLITE_OK)
	{
		LOGGER << "sqlite error: " << sqlite3_errmsg(DB_handle);
		exit(1);
	}
	if(sqlite3_busy_timeout(DB_handle, 2000) != SQLITE_OK){
		LOGGER << "sqlite error: " << sqlite3_errmsg(DB_handle);
		exit(1);
	}
	if(sqlite3_exec(DB_handle,
		"PRAGMA auto_vacuum = full;"     //move free pages to EOF and truncate at every commit
		"PRAGMA journal_mode = PERSIST;" //do not delete the journal file between uses
		"PRAGMA cache_size = 8192;"      //increase max in memory pages from 2000
		"PRAGMA temp_store = MEMORY;"    //make sure temp store is memory and not a file
		, NULL, NULL, NULL) != SQLITE_OK)
	{
		LOGGER << "sqlite error: " << sqlite3_errmsg(DB_handle);
	}
}

database::database(
	const database & DB
):
	DB_handle(DB.DB_handle),
	ref_cnt(DB.ref_cnt)
{
	++*ref_cnt;
}

database::~database()
{
	if(*ref_cnt == 0){
		if(sqlite3_close(DB_handle) != SQLITE_OK){
			LOGGER << "sqlite error: " << sqlite3_errmsg(DB_handle);
		}
	}else{
		--*ref_cnt;
	}
}

boost::int64_t database::blob_allocate(const std::string & query, const int & size)
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

void database::blob_close(blob & Blob)
{
	if(sqlite3_blob_close(Blob.blob_handle) != SQLITE_OK){
		LOGGER << "sqlite error: " << sqlite3_errmsg(DB_handle);
	}
}

void database::blob_open(blob & Blob, const bool & writeable)
{
	assert(Blob.rowid != 0);
	int tries = 0;
	while(sqlite3_blob_open(
		DB_handle,
		"main",         //symbolic DB name ("main" is default)
		Blob.table.c_str(),
		Blob.column.c_str(),
		Blob.rowid,          //row ID (primary key) of row with blob
		(int)writeable, //0 = read only, non-zero = read/write
		&Blob.blob_handle
	) != SQLITE_OK){
		if(tries++ >= 7){
			LOGGER << "8 consecutive blob open errors, aborting";
			exit(1);
		}else{
			LOGGER << sqlite3_errmsg(DB_handle);
		}
	}
}

void database::blob_read(blob & Blob, char * const buff, const int & size, const int & offset)
{
	boost::recursive_mutex::scoped_lock lock(*Mutex);
	blob_open(Blob, false);
	if(sqlite3_blob_read(Blob.blob_handle, (void *)buff, size, offset) != SQLITE_OK){
		LOGGER << sqlite3_errmsg(DB_handle);
		exit(1);
	}
	blob_close(Blob);
}

void database::blob_write(blob & Blob, const char * const buff, const int & size, const int & offset)
{
	boost::recursive_mutex::scoped_lock lock(*Mutex);
	blob_open(Blob, true);
	if(sqlite3_blob_write(Blob.blob_handle, (const void *)buff, size, offset) != SQLITE_OK){
		LOGGER << sqlite3_errmsg(DB_handle);
		exit(1);
	}
	blob_close(Blob);
}

int database::query(const std::string & query)
{
	boost::recursive_mutex::scoped_lock lock(*Mutex);
	int code;
	if((code = sqlite3_exec(DB_handle, query.c_str(), NULL, NULL, NULL)) != SQLITE_OK){
		LOGGER << "sqlite error " << code << ": " << sqlite3_errmsg(DB_handle) << " query: " << query;
	}
	return code;
}

int database::query(const std::string & query, int (*fun_ptr)(int, char **, char **))
{
	boost::recursive_mutex::scoped_lock lock(*Mutex);
	int code;
	if((code = sqlite3_exec(DB_handle, query.c_str(), fun_call_back_wrapper, (void *)fun_ptr, NULL)) != 0){
		LOGGER << "sqlite error " << code << ": " << sqlite3_errmsg(DB_handle) << " query: " << query;
	}
	return code;
}
