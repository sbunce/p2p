#include "download_file_conn.h"

download_file_conn::download_file_conn(download * download_in, const std::string & server_IP_in)
:
slot_ID_requested(false),
slot_ID_received(false),
close_slot_sent(false)
{
	Download = download_in;
	server_IP = server_IP_in;
}
