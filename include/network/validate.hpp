#ifndef VALIDATE
#define VALIDATE

//include
#include <boost/regex.hpp>

namespace network {
namespace {

//returns true if domain is valid
bool valid_domain(const std::string & domain)
{
	const boost::regex regex("[a-zA-Z0-9-]+(\\.[a-zA-Z0-9-])*");
	boost::cmatch what;
	return domain.size() <= 255 && boost::regex_match(domain.c_str(), what, regex);
}

//returns true if IP is valid
bool valid_IP(const std::string & IP)
{
	/* IP Regex
	source: Stephen Ryan of Dartware
	http://forums.dartware.com/viewtopic.php?t=452

	Conforms to RFC4291 section 2.2
	http://www.ietf.org/rfc/rfc4291.txt
	*/
	const boost::regex regex(
"^\\s*((([0-9A-Fa-f]{1,4}:){7}(([0-9A-Fa-f]{1,4})|:))|(([0-9A-Fa-f]{1,4}:){6}(:"
"|((25[0-5]|2[0-4]\\d|[01]?\\d{1,2})(\\.(25[0-5]|2[0-4]\\d|[01]?\\d{1,2})){3})|"
"(:[0-9A-Fa-f]{1,4})))|(([0-9A-Fa-f]{1,4}:){5}((:((25[0-5]|2[0-4]\\d|[01]?\\d{1"
",2})(\\.(25[0-5]|2[0-4]\\d|[01]?\\d{1,2})){3})?)|((:[0-9A-Fa-f]{1,4}){1,2})))|"
"(([0-9A-Fa-f]{1,4}:){4}(:[0-9A-Fa-f]{1,4}){0,1}((:((25[0-5]|2[0-4]\\d|[01]?\\d"
"{1,2})(\\.(25[0-5]|2[0-4]\\d|[01]?\\d{1,2})){3})?)|((:[0-9A-Fa-f]{1,4}){1,2}))"
")|(([0-9A-Fa-f]{1,4}:){3}(:[0-9A-Fa-f]{1,4}){0,2}((:((25[0-5]|2[0-4]\\d|[01]?"
"\\d{1,2})(\\.(25[0-5]|2[0-4]\\d|[01]?\\d{1,2})){3})?)|((:[0-9A-Fa-f]{1,4}){1,2"
"})))|(([0-9A-Fa-f]{1,4}:){2}(:[0-9A-Fa-f]{1,4}){0,3}((:((25[0-5]|2[0-4]\\d|[01"
"]?\\d{1,2})(\\.(25[0-5]|2[0-4]\\d|[01]?\\d{1,2})){3})?)|((:[0-9A-Fa-f]{1,4}){1"
",2})))|(([0-9A-Fa-f]{1,4}:)(:[0-9A-Fa-f]{1,4}){0,4}((:((25[0-5]|2[0-4]\\d|[01]"
"?\\d{1,2})(\\.(25[0-5]|2[0-4]\\d|[01]?\\d{1,2})){3})?)|((:[0-9A-Fa-f]{1,4}){1,"
"2})))|(:(:[0-9A-Fa-f]{1,4}){0,5}((:((25[0-5]|2[0-4]\\d|[01]?\\d{1,2})(\\.(25[0"
"-5]|2[0-4]\\d|[01]?\\d{1,2})){3})?)|((:[0-9A-Fa-f]{1,4}){1,2})))|(((25[0-5]|2["
"0-4]\\d|[01]?\\d{1,2})(\\.(25[0-5]|2[0-4]\\d|[01]?\\d{1,2})){3})))(%.+)?\\s*$"
	);
	boost::cmatch what;
	return boost::regex_match(IP.c_str(), what, regex);
}

//returns true if address is valid domain or IP
bool valid_address(const std::string & address)
{
	return valid_domain(address) || valid_IP(address);
}

bool valid_port(const std::string & port)
{
	return port.size() <= 5 && port.size() > 0 && atoi(port.c_str()) > 0
		&& atoi(port.c_str()) < 65536;
}
}//end unnamed namespace
}//end namespace network
#endif
