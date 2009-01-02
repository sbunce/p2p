#include "DB_hash_free.h"

atomic_bool DB_hash_free::program_start(true);

DB_hash_free::DB_hash_free()
: DB(global::DATABASE_PATH)
{
	DB.query("CREATE TABLE IF NOT EXISTS hash_free (offset TEXT, size TEXT, reserved INTEGER)");
	DB.query("CREATE INDEX IF NOT EXISTS hash_free_size_index ON hash_free (size)");

	/*
	Unreserve all space upon program start. It's possible that space was reserved,
	but not used, before the program shut down.
	*/
	if(program_start){
		DB.query("UPDATE hash_free SET reserved = 0");
		program_start = false;
	}
}
