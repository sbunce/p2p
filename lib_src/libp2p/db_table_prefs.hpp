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
	//get a value
	static unsigned get_max_download_rate(db::pool::proxy DB = db::pool::proxy());
	static unsigned get_max_connections(db::pool::proxy DB = db::pool::proxy());
	static unsigned get_max_upload_rate(db::pool::proxy DB = db::pool::proxy());
	static std::string get_ID(db::pool::proxy DB = db::pool::proxy());
	static std::string get_port(db::pool::proxy DB = db::pool::proxy());

	//set a value
	static void set_max_download_rate(const unsigned rate, db::pool::proxy DB = db::pool::proxy());
	static void set_max_connections(const unsigned connections, db::pool::proxy DB = db::pool::proxy());
	static void set_max_upload_rate(const unsigned rate, db::pool::proxy DB = db::pool::proxy());
	static void set_port(const std::string & port, db::pool::proxy DB = db::pool::proxy());

private:
	prefs(){}

	class static_wrap
	{
	public:
		class static_objects
		{
		public:
			//get value and do lexical cast to T
			template<typename T>
			T get(const std::string & key, db::pool::proxy DB)
			{
				boost::shared_ptr<cache_element> CE = get_cache_element(key);
				std::string value;
				{//BEGIN lock scope
				boost::mutex::scoped_lock lock(CE->mutex);
				if(CE->value.empty()){
					std::stringstream ss;
					ss << "SELECT value FROM prefs WHERE key = '" << key << "'";
					DB->query(ss.str(), boost::bind(&call_back, _1, _2, _3, boost::ref(value)));
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
			void set(const std::string & key, const T & value, db::pool::proxy DB)
			{
				std::stringstream ss;
				ss << value;
				boost::shared_ptr<cache_element> CE = get_cache_element(key);
				{//BEGIN lock scope
				boost::mutex::scoped_lock lock(CE->mutex);
				CE->value = ss.str();
				}//END lock scope
				ss.str(""); ss.clear();
				ss << "UPDATE prefs SET value = '" << value << "' WHERE key = '" << key << "'";
				DB->query(ss.str());
			}

		private:
			class cache_element : private boost::noncopyable
			{
			public:
				boost::mutex mutex;
				std::string value;
			};

			boost::mutex Cache_mutex;
			std::map<std::string, boost::shared_ptr<cache_element> > Cache;

			/*
			call_back:
				Call back used to get value.
			get_cache_element:
				Get cache element, create cache element if it doesn't exist.
			set_wrapper:
				Wraps database call. Needed so that db::connection object isn't tied
				up in thread pool.
			*/
			static int call_back(int columns, char ** response, char ** column_name,
				std::string & value);
			boost::shared_ptr<cache_element> get_cache_element(const std::string & key);
			static void set_wrapper(const std::string query);
		};

		//get access to static objects
		static static_objects & get();
	private:
		static boost::once_flag once_flag;
		static static_objects & _get();
	};
};
}//end of namespace table
}//end of namespace database
#endif
