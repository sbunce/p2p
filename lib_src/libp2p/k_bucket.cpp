#include "k_bucket.hpp"

//BEGIN contact
k_bucket::contact::contact(
	const std::string & remote_ID_in,
	const network::endpoint & endpoint_in
):
	remote_ID(remote_ID_in),
	endpoint(endpoint_in),
	ping_sent(false),
	last_seen(std::time(NULL))
{

}

k_bucket::contact::contact(const contact & C):
	endpoint(C.endpoint),
	ping_sent(C.ping_sent),
	last_seen(C.last_seen)
{

}

std::time_t k_bucket::contact::idle()
{
	return std::time(NULL) - last_seen;
}

void k_bucket::contact::touch()
{
	last_seen = std::time(NULL);
}
//END contact

k_bucket::k_bucket()
{

}
