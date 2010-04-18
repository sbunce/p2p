#ifndef H_DB_TABLE_PREFS
#define H_DB_TABLE_PREFS

//custom
#include "db_all.hpp"
#include "path.hpp"
#include "settings.hpp"

//include
#include <boost/optional.hpp>
#include <boost/thread.hpp>
#include <thread_pool.hpp>

//standard
#include <string>

namespace db{
namespace table{
class prefs
{
public:
	//causes all values to be cached
	static void warm_up_cache(db::pool::proxy DB = db::pool::proxy());

	//get a value
	static unsigned get_max_download_rate(
		db::pool::proxy DB = db::pool::proxy());
	static unsigned get_max_connections(
		db::pool::proxy DB = db::pool::proxy());
	static unsigned get_max_upload_rate(
		db::pool::proxy DB = db::pool::proxy());
	static std::string get_ID(
		db::pool::proxy DB = db::pool::proxy());
	static std::string get_port(
		db::pool::proxy DB = db::pool::proxy());

	//set a value
	static void set_max_download_rate(const unsigned rate,
		db::pool::proxy DB = db::pool::proxy());
	static void set_max_connections(const unsigned connections,
		db::pool::proxy DB = db::pool::proxy());
	static void set_max_upload_rate(const unsigned rate,
		db::pool::proxy DB = db::pool::proxy());
	static void set_port(const std::string & port,
		db::pool::proxy DB = db::pool::proxy());

private:
	prefs(){}

	class static_wrap
	{
	public:
		class static_objects
		{
		public:
			static_objects():
				Thread_Pool(1)
			{}

			//each cached value has a lock to protect it
			boost::mutex max_download_rate_mutex;
			boost::optional<unsigned> max_download_rate;
			boost::mutex max_connections_mutex;
			boost::optional<unsigned> max_connections;
			boost::mutex max_upload_rate_mutex;
			boost::optional<unsigned> max_upload_rate;
			boost::mutex ID_mutex;
			boost::optional<std::string> ID;
			boost::mutex port_mutex;
			boost::optional<std::string> port;

			//used for async set
			thread_pool Thread_Pool;
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
