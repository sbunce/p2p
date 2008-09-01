#ifndef H_DOWNLOAD_TRACKER
#define H_DOWNLOAD_TRACKER

//custom
#include "DB_blacklist.h"
#include "download.h"
#include "global.h"

//std
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

class download_tracker : public download
{
public:
	download_tracker();
	~download_tracker();

	//documentation for virtual functions in abstract base class
	virtual bool complete();
	virtual download::mode request(const int & socket, std::string & request, std::vector<std::pair<char, int> > & expected, int & slots_used);
	virtual void response(const int & socket, std::string block);
	virtual void stop();

private:
	bool download_complete;
};
#endif
