#ifndef H_FIXED_STATUSBAR
#define H_FIXED_STATUSBAR

//custom
#include "settings.hpp"

//include
#include <boost/utility.hpp>
#include <convert.hpp>
#include <gtkmm.h>
#include <p2p.hpp>

//standard
#include <iomanip>
#include <sstream>
#include <string>

class fixed_statusbar : public Gtk::Fixed, private boost::noncopyable
{
public:
	fixed_statusbar(p2p & P2P_in);

private:
	p2p & P2P;

	Gtk::Label * download_label;
	Gtk::Label * upload_label;

	/*
	refresh:
		Update info.
	*/
	bool refresh();
};
#endif
