#include "download_file_conn.h"

download_file_conn::download_file_conn(download * download_in, const std::string & server_IP_in, const unsigned int & file_ID_in)
:
file_ID(file_ID_in)
{
	Download = download_in;
	server_IP = server_IP_in;
}
