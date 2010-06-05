#ifndef H_WINDOW_TRANSFER_INFO
#define H_WINDOW_TRANSFER_INFO

//include
#include <boost/cstdint.hpp>
#include <boost/utility.hpp>
#include <gtkmm.h>
#include <p2p.hpp>

//standard
#include <string>

class window_transfer_info : public Gtk::ScrolledWindow, private boost::noncopyable
{
public:
	enum type_t {
		download,
		upload
	};

	window_transfer_info(
		const type_t type_in,
		p2p & P2P_in,
		const std::string & hash_in
	);

private:
	const type_t type;
	p2p & P2P;
	const std::string hash;


};
#endif
