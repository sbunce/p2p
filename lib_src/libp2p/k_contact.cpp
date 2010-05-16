#include "k_contact.hpp"

k_contact::k_contact(
	const unsigned touch_timeout_in,
	const unsigned delay
):
	touch_timeout(touch_timeout_in),
	time(std::time(NULL) + delay),
	sent(false),
	timeout_cnt(0)
{

}

k_contact::k_contact(const k_contact & KC):
	touch_timeout(KC.touch_timeout),
	time(KC.time),
	sent(KC.sent),
	timeout_cnt(KC.timeout_cnt)
{

}

bool k_contact::send()
{
	if(!sent && std::time(NULL) > time){
		time = std::time(NULL) + protocol_udp::response_timeout;
		sent = true;
		return true;
	}
	return false;
}

bool k_contact::send_immediate()
{
	if(!sent){
		time = std::time(NULL) + protocol_udp::response_timeout;
		sent = true;
		return true;
	}
	return false;
}

bool k_contact::timeout()
{
	if(sent && std::time(NULL) > time){
		touch();
		++timeout_cnt;
		return true;
	}
	return false;
}

unsigned k_contact::timeout_count()
{
	return timeout_cnt;
}

void k_contact::touch()
{
	time = std::time(NULL) + touch_timeout;
	sent = false;
}
