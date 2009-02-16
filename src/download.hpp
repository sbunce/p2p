//NOT-THREADSAFE
#ifndef H_DOWNLOAD
#define H_DOWNLOAD

//custom
#include "download_connection.hpp"
#include "download_info.hpp"
#include "global.hpp"
#include "speed_calculator.hpp"

//std
#include <list>
#include <string>
#include <map>
#include <vector>

class download : private boost::noncopyable
{
public:
	download();
	virtual ~download();

	enum mode { BINARY_MODE, TEXT_MODE, NO_REQUEST };

	/* Regular Virtuals
	is_cancelled:
		Returns download::cancel to indicate to the download factory upon
		transition if the download was cancelled. If not defined in derived this
		always return false.
	<register>|<unregister>_connection:
		The p2p_buffer registers or unregisters a connection that has the file
		with these functions. These are defined in both download_hash_tree and
		download file to keep track of specific server information. However, both
		download_hash_tree and download_file call these.
	*/
	virtual bool is_cancelled();
	virtual void register_connection(const download_connection & DC);
	virtual void unregister_connection(const int & socket);

	/* Pure Virtuals
	complete:
		If true is returned the download is removed.
	get_download_info:
		This is used for transitioning from a download_hash_tree to a
		download_file. No other download types will need this.
	hash:
		Returns the root hash for the download.
	name:
		Returns the name of the download to be displayed in GUI.
	percent_complete:
		Returns the percentage (0-100) complete.
	request:
		Make a request of a server, expected vector is possible response commands
		paired with length of responses
	response:
		Responses to requests are sent to this function once the entire response
		is gotten.
	size:
		Returns the size (bytes) of the download.
	stop:
		This must prepare the download for early termination and make complete()
		return true A.S.A.P
	*/
	virtual bool complete() = 0;
	virtual download_info get_download_info() = 0;
	virtual const std::string hash() = 0;
	virtual const std::string name() = 0;
	virtual unsigned percent_complete() = 0;
	virtual mode request(const int & socket, std::string & request, std::vector<std::pair<char, int> > & expected, int & slots_used) = 0;
	virtual void response(const int & socket, std::string block) = 0;
	virtual const boost::uint64_t size() = 0;
	virtual void stop() = 0;

	/* Functions that p2p_buffer interacts with.
	connection_count:
		Returns how many connections are associated with this download.
	is_visible:
		Returns download::visible. If returns true then the download is displayed
		in the GUI.
	servers:
		Returns IP address and upload speed for each server.
	speed:
		Returns total speed of this download.
	update_speed:
		Used by p2p_buffer to update the download speed for a specific server.
	*/
	int connection_count();
	bool is_visible();
	void servers(std::vector<std::string> & IP, std::vector<unsigned> & speed);
	unsigned speed();
	void update_speed(const int & socket, const int & n_bytes);

protected:
	//returned by is_cancelled and is_visible
	bool cancel;
	bool visible;

	/*
	When update_speed is called the byte count is added to this total. This can
	be used by derived downloads to calculate percentage complete.
	*/
	boost::uint64_t bytes_received;

private:
	class server_info
	{
	public:
		server_info(const std::string & IP_in): IP(IP_in){}
		std::string IP;
		speed_calculator Speed_Calculator;
	};

	/*
	client_buffers register/unregister with the download by storing information
	here. This maps the socket number to the server IP.
	*/
	std::map<int, server_info> Connection;

	//total speed for the download
	speed_calculator Speed_Calculator;
};
#endif
