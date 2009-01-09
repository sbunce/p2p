/*
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
*/
#ifndef H_SQLITE3_WRAPPER
#define H_SQLITE3_WRAPPER

//boost
#include <boost/thread/recursive_mutex.hpp>

//custom
#include "global.h"

//sqlite
#include <sqlite3.h>

//std
#include <exception>
#include <iostream>

class sqlite3_wrapper
{
public:
	class sqlite3_wrapper_exception: public std::exception
	{
		virtual const char * what() const throw()
		{
			return "failed to open sqlite database";
		}
	}SWE;

	sqlite3_wrapper(const std::string & DB_path)
	{
		int code;
		if(code = sqlite3_open(DB_path.c_str(), &sqlite3_DB) != 0){
			LOGGER << "sqlite error: " << code << " " << sqlite3_errmsg(sqlite3_DB);
			throw SWE;
		}
		if(code = sqlite3_busy_timeout(sqlite3_DB, global::DB_TIMEOUT) != 0){
			LOGGER << "sqlite error: " << code << " " << sqlite3_errmsg(sqlite3_DB);
			throw SWE;
		}
	}

	//query with no call back
	int query(const std::string & query)
	{
		boost::recursive_mutex::scoped_lock lock(Mutex);
		int code;
		if(code = sqlite3_exec(sqlite3_DB, query.c_str(), NULL, NULL, NULL) != 0){
			LOGGER << "sqlite error: " << code << " " << sqlite3_errmsg(sqlite3_DB) << " query: " << query;
			#ifndef NDEBUG
			exit(1);
			#endif
		}
		return code;
	}

	//query with function call back
	int query(const std::string & query, int (*fun_ptr)(int, char **, char **))
	{
		boost::recursive_mutex::scoped_lock lock(Mutex);
		int code;
		if(code = sqlite3_exec(sqlite3_DB, query.c_str(), fun_call_back_wrapper, (void *)fun_ptr, NULL) != 0){
			LOGGER << "sqlite error: " << code << " " << sqlite3_errmsg(sqlite3_DB) << " query: " << query;
			#ifndef NDEBUG
			exit(1);
			#endif
		}
		return code;
	}

	//query with function call back and object
	template <typename T>
	int query(const std::string & query, int (*fun_ptr)(T, int, char **, char **), T t)
	{
		boost::recursive_mutex::scoped_lock lock(Mutex);
		//std::pair<func signature, object>
		std::pair<int (*)(T, int, char **, char **), T*> call_back_info(fun_ptr, &t);
		int code;
		if(code = sqlite3_exec(sqlite3_DB, query.c_str(), fun_with_object_call_back_wrapper<T>,
			(void *)&call_back_info, NULL) != 0)
		{
			LOGGER << "sqlite error: " << code << " " << sqlite3_errmsg(sqlite3_DB) << " query: " << query;
			#ifndef NDEBUG
			exit(1);
			#endif
		}
		return code;
	}

	//query with function call back and object by reference
	template <typename T>
	int query(const std::string & query, int (*fun_ptr)(T&, int, char **, char **), T & t)
	{
		boost::recursive_mutex::scoped_lock lock(Mutex);
		//std::pair<func signature, object>
		std::pair<int (*)(T&, int, char **, char **), T*> call_back_info(fun_ptr, &t);
		int code;
		if(code = sqlite3_exec(sqlite3_DB, query.c_str(), fun_with_object_reference_call_back_wrapper<T>,
			(void *)&call_back_info, NULL) != 0)
		{
			LOGGER << "sqlite error: " << code << " " << sqlite3_errmsg(sqlite3_DB) << " query: " << query;
			#ifndef NDEBUG
			exit(1);
			#endif
		}
		return code;
	}

	//query with member function call back
	template <typename T>
	int query(const std::string & query, T * t, int (T::*memfun_ptr)(int, char **, char **))
	{
		boost::recursive_mutex::scoped_lock lock(Mutex);
		//std::pair<object with func, func signature>
		std::pair<T*, int (T::*)(int, char **, char **)> call_back_info(t, memfun_ptr);
		int code;
		if(code = sqlite3_exec(sqlite3_DB, query.c_str(), memfun_call_back_wrapper<T>,
			(void *)&call_back_info, NULL) != 0)
		{
			LOGGER << "sqlite error: " << code << " " << sqlite3_errmsg(sqlite3_DB) << " query: " << query;
			#ifndef NDEBUG
			exit(1);
			#endif
		}
		return code;
	}

	//query with member function call back and object
	template <typename T, typename T_obj>
	int query(const std::string & query, T * t, int (T::*memfun_ptr)(T_obj, int, char **, char **), T_obj T_Obj)
	{
		boost::recursive_mutex::scoped_lock lock(Mutex);
		/*
		std::pair<std::pair<object with func, func signature>, object> >
		A boost::tuple would be nice here but I want to leave this stdlib only.
		*/
		std::pair<std::pair<T*, int (T::*)(T_obj, int, char **, char **)>, T_obj*>
			call_back_info(std::make_pair(t, memfun_ptr), &T_Obj);
		int code;
		if(code = sqlite3_exec(sqlite3_DB, query.c_str(), memfun_with_object_call_back_wrapper<T, T_obj>,
			(void *)&call_back_info, NULL) != 0)
		{
			LOGGER << "sqlite error: " << code << " " << sqlite3_errmsg(sqlite3_DB) << " query: " << query;
			#ifndef NDEBUG
			exit(1);
			#endif
		}
		return code;
	}

	//query with member function call back and object by reference
	template <typename T, typename T_obj>
	int query(const std::string & query, T * t, int (T::*memfun_ptr)(T_obj&, int, char **, char **), T_obj & T_Obj)
	{
		boost::recursive_mutex::scoped_lock lock(Mutex);
		/*
		std::pair<std::pair<object with func, func signature>, object> >
		A boost::tuple would be nice here but I want to leave this stdlib only.
		*/
		std::pair<std::pair<T*, int (T::*)(T_obj&, int, char **, char **)>, T_obj*>
			call_back_info(std::make_pair(t, memfun_ptr), &T_Obj);
		int code;
		if(code = sqlite3_exec(sqlite3_DB, query.c_str(), memfun_with_object_reference_call_back_wrapper<T, T_obj>,
			(void *)&call_back_info, NULL) != 0)
		{
			LOGGER << "sqlite error: " << code << " " << sqlite3_errmsg(sqlite3_DB) << " query: " << query;
			#ifndef NDEBUG
			exit(1);
			#endif
		}
		return code;
	}

private:
	sqlite3 * sqlite3_DB;

	/*
	Mutex for all public functions. A recursive mutex is used because a thread
	might want to do a query inside a call back.
	*/
	boost::recursive_mutex Mutex;

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
#endif
