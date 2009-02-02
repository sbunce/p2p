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

	/*
	If the is_visible() function returns true the client sends information from all
	the functions in this section to the GUI. If is_visible is set to true in derived
	but the rest of these functions aren't defined in derived then funny values to
	make the mistake obvious will be returned (see implementation for specifics).

	hash             - returns a unique identifier to the download (may be a hash)
	name             - returns the name of this download
	percent_complete - should return integer 0 to 100 to indicate percent complete
	speed            - download speed in bytes per second
	size             - the total amount needing to be downloaded
	*/
	virtual bool is_visible();
	virtual const std::string hash();
	virtual const std::string name();
	virtual unsigned percent_complete();
	virtual unsigned speed();
	virtual const boost::uint64_t size();

	/*
	If server specific information needs to be stored in the download these can
	be overridden. If you do override these you must call the register and unregister
	functions in the base class from the derived class. (see download_file for example)
	*/
	virtual void register_connection(const download_connection & DC);
	virtual void unregister_connection(const int & socket);

	/*
	This can be used to communicate to the download factory. You can have the
	download_factory start another download if this download was not cancelled
	for example.

	If not defined in derived this always returns false.
	*/
	virtual bool is_cancelled();

	/*
	This is used for transitioning from a download_hash_tree to a download_file.
	No other download types will need this. If not defined this will return
	funny values to make the mistake obvious (see implementation for specifics).
	*/
	virtual download_info get_download_info();

	/*
	Minimum functions which need to be defined for a download.

	complete - if true is returned the download is removed
	request  - make a request of a server, expected vector is possible response commands paired with length of responses
	response - responses to requests are sent to this function once the entire response is gotten
	stop     - this must prepare the download for early termination and make complete() return true A.S.A.P
	*/
	virtual bool complete() = 0;
	virtual mode request(const int & socket, std::string & request, std::vector<std::pair<char, int> > & expected, int & slots_used) = 0;
	virtual void response(const int & socket, std::string block) = 0;
	virtual void stop() = 0;

	/*
	Functions that the client_buffer interacts with.

	upload_speed     - updates the server specific speed_calculators whenever bytes are received
	server_info      - populates the first vector with IP's and the second with speeds
	connection_count - returns number of connections registered
	*/
	void update_speed(const int & socket, const int & n_bytes);
	void servers(std::vector<std::string> & IP, std::vector<unsigned> & speed);
	int connection_count();

protected:
	//returned by is_cancelled and is_visible
	bool cancel;
	bool visible;

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
