#ifndef H_P2P
#define H_P2P

//include
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
	class transfer
	{
	public:
		std::string hash;          //root hash of hash tree
		std::string name;          //name of the file
		boost::uint64_t file_size; //size of the file (bytes)
		boost::uint64_t tree_size; //size of hash tree (bytes)
		unsigned percent_complete; //0-100
		unsigned download_peers;   //number of hosts we're downloading from
		unsigned download_speed;   //total download bytes/second
		unsigned upload_peers;     //number of hosts we're uploading to
		unsigned upload_speed;     //total upload bytes/second
	};

	//needed to start a download
	class download
	{
	public:
		download(
			const std::string & hash_in,
			const std::string & name_in
		):
			hash(hash_in),
			name(name_in)
		{}
		std::string hash;
		std::string name;
	};

	/* Transfers
	remove_download:
		Removes a running download.
	start_download:
		Starts a download.
	transfers:
		Clears T and populates it with transfer information.
	*/
	void remove_download(const std::string & hash);
	void start_download(const p2p::download & D);
	void transfers(std::vector<p2p::transfer> & T);

	/* Info
	download_rate:
		Returns current download rate (B/s).
	share_size_bytes:
		Size of all shared files.
	share_size_files:
		The number of files shared.
	upload_rate:
		Returns current upload rate (B/s).
	*/
	unsigned download_rate();
	boost::uint64_t share_size_bytes();
	boost::uint64_t share_size_files();
	unsigned upload_rate();

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

	/* UNSTABLE - MAY BE REMOVED FROM API AT ANYTIME
	test:
		Stores data files in a different location for testing purposes.
		Note: Must be called before instantiating p2p.
	*/
	static void test(const std::string & port, const std::string & program_directory);

private:
	boost::shared_ptr<p2p_impl> P2P_impl;
};
#endif
