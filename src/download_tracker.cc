#include "download_tracker.h"

download_tracker::download_tracker():
	download_complete(false)
{

}

download_tracker::~download_tracker()
{

}

bool download_tracker::complete()
{
	return download_complete;
}

download::mode download_tracker::request(const int & socket, std::string & request, std::vector<std::pair<char, int> > & expected)
{
	request = "T";
	return download::TEXT_MODE;
}

void download_tracker::response(const int & socket, std::string block)
{

}

void download_tracker::stop()
{

}
