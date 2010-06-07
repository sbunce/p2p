#ifndef H_P2P
#define H_P2P

//include
#include <boost/optional.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>

//standard
#include <list>
#include <string>

class p2p_impl;
class p2p : private boost::noncopyable
{
public:
	p2p();

	//status of a transfer
	class transfer_info
	{
	public:
		std::string hash;               //hash file tracked by on DHT
		std::string name;               //name of file
		boost::uint64_t tree_size;      //size of hash tree (bytes)
		boost::uint64_t file_size;      //size of file (bytes), 0 if not known
		unsigned percent_complete;      //0-100 (tree and file combined)
		unsigned tree_percent_complete; //0-100
		unsigned file_percent_complete; //0-100
		unsigned download_peers;        //number of hosts we're downloading from
		unsigned upload_peers;          //number of hosts we're uploading to
		unsigned download_speed;        //total download bytes/second
		unsigned upload_speed;          //total upload bytes/second

		//individual hosts
		class host_element
		{
		public:
			std::string IP;
			std::string port;
			unsigned download_speed;
			unsigned upload_speed;
		};
		std::list<host_element> host;
	};

	//needed to start a download
	class download_info
	{
	public:
		download_info(
			const std::string & hash_in,
			const std::string & name_in
		);
		std::string hash;
		std::string name;
	};

	/* Transfers
	remove_download:
		Removes a running download.
	start_download:
		Starts a download.
	*/
	void remove_download(const std::string & hash);
	void start_download(const p2p::download_info & DI);

	/* Info
	connection:
		Returns number of hosts we're connected to.
	DHT_count:
		Returns number of contacts in DHT routing table.
	share_size:
		Size of all shared files (bytes).
	share_files:
		The number of files shared.
	transfer (0 parameters):
		Returns information for all transfers.
	transfer (1 parameter):
		Returns information for specific transfer.
	TCP_download_rate:
		Returns current average TCP download rate (B/s).
	TCP_upload_rate:
		Returns current average TCP upload rate (B/s).
	UDP_download_rate:
		Returns current average UDP download rate (B/s).
	UDP_upload_rate:
		Returns current average UDP upload rate (B/s).
	*/
	unsigned connections();
	unsigned DHT_count();
	boost::uint64_t share_size();
	boost::uint64_t share_files();
	std::list<transfer_info> transfer();
	boost::optional<transfer_info> transfer(const std::string & hash);
	unsigned TCP_download_rate();
	unsigned TCP_upload_rate();
	unsigned UDP_download_rate();
	unsigned UDP_upload_rate();

	/* Get Options
	get_download_rate:
		Returns current download rate (B/s).
	get_max_connections:
		Returns maximum allowed connections.
	get_max_download_rate:
		Returns maximum allowed download rate (B/s).
	get_max_upload_rate:
		Returns maximum allowed upload rate (B/s).
	*/

	unsigned get_max_connections();
	unsigned get_max_download_rate();
	unsigned get_max_upload_rate();

	/* Set Options
	set_max_download_rate:
		Set maximum allowed download rate (B/s).
	set_max_connections:
		Set maximum allowed connections.
	set_max_upload_rate:
		Set maximum allowed upload rate (B/s).
	*/
	void set_max_download_rate(const unsigned rate);
	void set_max_connections(const unsigned connections);
	void set_max_upload_rate(const unsigned rate);

	/* Change Defaults
	Note: These must be called before p2p instantiated.
	set_db_file_name:
		Change the name of the database file within program directory.
	set_program_dir:
		Change the program directory.
		Note: Must be called before instantiating p2p.
	*/
	static void set_db_file_name(const std::string & name);
	static void set_program_dir(const std::string & path);

private:
	boost::shared_ptr<p2p_impl> P2P_impl;
};
#endif
