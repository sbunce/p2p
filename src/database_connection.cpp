#include "database_connection.hpp"

speed_calculator database::connection::Query;

database::connection::connection():
	ref_cnt(new atomic_int<int>(0)),
	Mutex(new boost::recursive_mutex())
{
	if(sqlite3_open_v2("DB", &DB_handle,
		SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX,
		0) != SQLITE_OK)
	{
		LOGGER << "sqlite error: " << sqlite3_errmsg(DB_handle);
		exit(1);
	}

	//timeout in ms, this can not be zero with concurrency
	if(sqlite3_busy_timeout(DB_handle, 4000) != SQLITE_OK){
		LOGGER << "sqlite error: " << sqlite3_errmsg(DB_handle);
		exit(1);
	}

	//move free pages to EOF and truncate at every commit
	if(sqlite3_exec(DB_handle, "PRAGMA auto_vacuum = full;" , NULL, NULL, NULL) != SQLITE_OK){
		LOGGER << "sqlite error: " << sqlite3_errmsg(DB_handle);
	}

	database::init::run();
}

database::connection::connection(const database::connection & DB)
{
	boost::recursive_mutex::scoped_lock lock(*DB.Mutex);
	copy(DB);
}

database::connection & database::connection::operator = (const database::connection & rval)
{
	boost::recursive_mutex::scoped_lock lock(*rval.Mutex);
	copy(rval);
	return *this;
}

void database::connection::copy(const database::connection & DB)
{
	DB_handle = DB.DB_handle;
	ref_cnt = DB.ref_cnt;
	Mutex = DB.Mutex;
	++*ref_cnt;
}

database::connection::~connection()
{
	if(*ref_cnt == 0){
		if(sqlite3_close(DB_handle) != SQLITE_OK){
			LOGGER << "sqlite error: " << sqlite3_errmsg(DB_handle);
		}
	}else{
		--*ref_cnt;
	}
}

boost::int64_t database::connection::blob_allocate(const std::string & query, const int & size)
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
	Query.update(1);
	return sqlite3_last_insert_rowid(DB_handle);
}

void database::connection::blob_close(sqlite3_blob * blob_handle)
{
	if(sqlite3_blob_close(blob_handle) != SQLITE_OK){
		LOGGER << "sqlite error: " << sqlite3_errmsg(DB_handle);
		exit(1);
	}
}

sqlite3_blob * database::connection::blob_open(blob & Blob, const bool & writeable)
{
	assert(Blob.rowid != 0);
	sqlite3_blob * blob_handle;
	int code;
	while((code = sqlite3_blob_open(
		DB_handle,
		"main",              //symbolic DB name ("main" is default)
		Blob.table.c_str(),
		Blob.column.c_str(),
		Blob.rowid,          //row ID (primary key) of row with blob
		(int)writeable,      //0 = read only, non-zero = read/write
		&blob_handle
	)) != SQLITE_OK){
		Query.update(1);
		if(code == SQLITE_BUSY){
			LOGGER << "sqlite yield on SQLITE_BUSY";
			boost::this_thread::yield();
		}else{
			LOGGER << "sqlite error " << code << ": " << sqlite3_errmsg(DB_handle);
			break;
		}
	}
	Query.update(1);
	return blob_handle;
}

void database::connection::blob_read(blob & Blob, char * const buff, const int & size, const int & offset)
{
	boost::recursive_mutex::scoped_lock lock(*Mutex);
	sqlite3_blob * blob_handle = blob_open(Blob, false);
	if(sqlite3_blob_read(blob_handle, (void *)buff, size, offset) != SQLITE_OK){
		LOGGER << sqlite3_errmsg(DB_handle);
		exit(1);
	}
	blob_close(blob_handle);
	Query.update(1);
}

void database::connection::blob_write(blob & Blob, const char * const buff, const int & size, const int & offset)
{
	boost::recursive_mutex::scoped_lock lock(*Mutex);
	sqlite3_blob * blob_handle = blob_open(Blob, true);
	if(sqlite3_blob_write(blob_handle, (const void *)buff, size, offset) != SQLITE_OK){
		LOGGER << sqlite3_errmsg(DB_handle);
		exit(1);
	}
	blob_close(blob_handle);
	Query.update(1);
}

int database::connection::fun_call_back_wrapper(void * obj_ptr, int columns, char ** response, char ** column_name)
{
	int (*fun_ptr)(int, char **, char **) = (int (*)(int, char **, char **))obj_ptr;
	return fun_ptr(columns, response, column_name);
}

int database::connection::query(const std::string & query)
{
	boost::recursive_mutex::scoped_lock lock(*Mutex);
	int code;
	while((code = sqlite3_exec(DB_handle, query.c_str(), NULL, NULL, NULL)) != SQLITE_OK){
		Query.update(1);
		if(code == SQLITE_BUSY){
			LOGGER << "sqlite yield on SQLITE_BUSY" << " query: " << query;
			boost::this_thread::yield();
		}else{
			LOGGER << "sqlite error " << code << ": " << sqlite3_errmsg(DB_handle) << " query: " << query;
			break;
		}
	}
	Query.update(1);
	return code;
}

int database::connection::query(const std::string & query, int (*fun_ptr)(int, char **, char **))
{
	boost::recursive_mutex::scoped_lock lock(*Mutex);
	int code;
	while((code = sqlite3_exec(DB_handle, query.c_str(), fun_call_back_wrapper, (void *)fun_ptr, NULL)) != SQLITE_OK){
		Query.update(1);
		if(code == SQLITE_BUSY){
			LOGGER << "sqlite yield on SQLITE_BUSY" << " query: " << query;
			boost::this_thread::yield();
		}else{
			LOGGER << "sqlite error " << code << ": " << sqlite3_errmsg(DB_handle) << " query: " << query;
			break;
		}
	}
	Query.update(1);
	return code;
}
