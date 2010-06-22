#ifndef H_DB_TABLE_PREFS
#define H_DB_TABLE_PREFS

//custom
#include "db_all.hpp"
#include "path.hpp"
#include "settings.hpp"

//include
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <boost/thread.hpp>
#include <singleton.hpp>
#include <thread_pool.hpp>

//standard
#include <string>

/*
This class implements a caching mechanism which makes it so that the get_*
functions only do I/O the first time you call them.
*/
namespace db{
namespace table{
class prefs
{
public:
	//get value
	static unsigned get_max_download_rate();
	static unsigned get_max_connections();
	static unsigned get_max_upload_rate();
	static std::string get_ID();
	static std::string get_port();

	//set value
	static void set_max_download_rate(const unsigned rate);
	static void set_max_connections(const unsigned connections);
	static void set_max_upload_rate(const unsigned rate);
	static void set_port(const std::string & port);

	//get all values in cache
	static void init_cache();

private:
	prefs(){}

	class cache : public singleton_base<cache>
	{
		friend class singleton_base<cache>;
	public:
		//get value and do lexical cast to T
		template<typename T>
		T get(const std::string & key)
		{
			boost::shared_ptr<cache_element> CE = get_cache_element(key);
			std::string value;
			{//BEGIN lock scope
			boost::mutex::scoped_lock lock(CE->mutex);
			if(CE->value.empty()){
				std::stringstream ss;
				ss << "SELECT value FROM prefs WHERE key = '" << key << "'";
				db::pool::singleton()->get()->query(ss.str(),
					boost::bind(&call_back, _1, _2, _3, boost::ref(value)));
				assert(!value.empty());
				CE->value = value;
			}else{
				value = CE->value;
			}
			}//END lock scope
			try{
				return boost::lexical_cast<T>(value);
			}catch(const boost::bad_lexical_cast & e){
				LOG << "key: \"" << key << "\" " << e.what();
				exit(1);
			}
		}

		//set value
		template<typename T>
		void set(const std::string & key, const T & value)
		{
			std::stringstream ss;
			ss << value;
			boost::shared_ptr<cache_element> CE = get_cache_element(key);
			{//BEGIN lock scope
			boost::mutex::scoped_lock lock(CE->mutex);
			if(CE->value == ss.str()){
				//no need to write to database, value is the same as before
				return;
			}
			CE->value = ss.str();
			}//END lock scope
			ss.str(""); ss.clear();
			ss << "UPDATE prefs SET value = '" << value << "' WHERE key = '" << key << "'";
			Thread_Pool.enqueue(boost::bind(&cache::set_query, this, ss.str()));
		}

	private:
		cache():
			DB_Pool(db::pool::singleton()),
			Thread_Pool(this)
		{}

		//locks access to this object
		boost::mutex Mutex;

		/*
		Each cache element has it's own mutex so we don't have to hold the lock
		on Cache while doing I/O for a specific key/value pair.
		*/
		class cache_element : private boost::noncopyable
		{
		public:
			boost::mutex mutex;
			std::string value;
		};
		std::map<std::string, boost::shared_ptr<cache_element> > Cache;

		/*
		call_back:
			Call back used to get value.
		get_cache_element:
			Get cache element, create cache element if it doesn't exist.
		set_call_back:
			Called by thread pool to do query.
		*/
		static int call_back(int columns, char ** response, char ** column_name,
			std::string & value);
		boost::shared_ptr<cache_element> get_cache_element(const std::string & key);
		void set_query(const std::string query);

		/*
		This singleton depends on the db::pool singleton. To prevent static
		initialization order problems we keep a shared_ptr to db::pool.
		*/
		boost::shared_ptr<db::pool> DB_Pool;

		thread_pool_global Thread_Pool;
	};
};
}//end of namespace table
}//end of namespace database
#endif
