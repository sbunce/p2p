//include
#include <boost/regex.hpp>
#include <logger.hpp>

boost::regex domain_regex("[a-zA-Z0-9-]+(\\.[a-zA-Z0-9-])*");

/*
source: Stephen Ryan of Dartware
http://forums.dartware.com/viewtopic.php?t=452

Conforms to RFC4291 section 2.2
http://www.ietf.org/rfc/rfc4291.txt
*/
boost::regex IP_regex(
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

bool is_domain(const std::string & domain)
{
	boost::cmatch what;
	return domain.size() <= 255 && boost::regex_match(domain.c_str(), what, domain_regex);
}

bool is_IP(const std::string & address)
{
	boost::cmatch what;
	return boost::regex_match(address.c_str(), what, IP_regex);
}

int main()
{
	assert(!is_domain(""));
	assert(is_domain("a"));
	assert(is_domain("a.b"));
	assert(!is_domain(".a.b"));

	assert(!is_IP(""));
	assert(is_IP("1.2.3.4"));
	assert(!is_IP("1.2.3"));
	assert(is_IP("2001:0000:1234:0000:0000:C1C0:ABCD:0876"));
	assert(is_IP("3ffe:0b00:0000:0000:0001:0000:0000:000a"));
	assert(is_IP("FF02:0000:0000:0000:0000:0000:0000:0001"));
	assert(is_IP("0000:0000:0000:0000:0000:0000:0000:0001"));
	assert(is_IP("0000:0000:0000:0000:0000:0000:0000:0000"));
	assert(is_IP("::ffff:192.168.1.26"));

	//extra 0 not allowed
	assert(!is_IP("02001:0000:1234:0000:0000:C1C0:ABCD:0876"));

	//extra 0 not allowed
	assert(!is_IP("2001:0000:1234:0000:00001:C1C0:ABCD:0876"));

	assert(is_IP(" 2001:0000:1234:0000:0000:C1C0:ABCD:0876"));
	assert(is_IP(" 2001:0000:1234:0000:0000:C1C0:ABCD:0876  "));
	assert(!is_IP(" 2001:0000:1234:0000:0000:C1C0:ABCD:0876  0"));
	assert(!is_IP("2001:0000:1234: 0000:0000:C1C0:ABCD:0876"));
	assert(!is_IP("3ffe:0b00:0000:0001:0000:0000:000a"));
	assert(!is_IP("FF02:0000:0000:0000:0000:0000:0000:0000:0001"));
	assert(!is_IP("3ffe:b00::1::a"));
	assert(is_IP("2::10"));
	assert(is_IP("ff02::1"));
	assert(is_IP("fe80::"));
	assert(is_IP("2002::"));
	assert(is_IP("2001:db8::"));
	assert(is_IP("2001:0db8:1234::"));
	assert(is_IP("::ffff:0:0"));
	assert(is_IP("::1"));
	assert(is_IP("::ffff:192.168.1.1"));
	assert(is_IP("1:2:3:4:5:6:7:8"));
	assert(is_IP("1:2:3:4:5:6::8"));
	assert(is_IP("1:2:3:4:5::8"));
	assert(is_IP("1:2:3:4::8"));
	assert(is_IP("1:2:3::8"));
	assert(is_IP("1:2::8"));
	assert(is_IP("1::8"));
	assert(is_IP("1::2:3:4:5:6:7"));
	assert(is_IP("1::2:3:4:5:6"));
	assert(is_IP("1::2:3:4:5"));
	assert(is_IP("1::2:3:4"));
	assert(is_IP("1::2:3"));
	assert(is_IP("1::8"));
	assert(is_IP("::2:3:4:5:6:7:8"));
	assert(is_IP("::2:3:4:5:6:7"));
	assert(is_IP("::2:3:4:5:6"));
	assert(is_IP("::2:3:4:5"));
	assert(is_IP("::2:3:4"));
	assert(is_IP("::2:3"));
	assert(is_IP("::8"));
	assert(is_IP("1:2:3:4:5:6::"));
	assert(is_IP("1:2:3:4:5::"));
	assert(is_IP("1:2:3:4::"));
	assert(is_IP("1:2:3::"));
	assert(is_IP("1:2::"));
	assert(is_IP("1::"));
	assert(is_IP("1:2:3:4:5::7:8"));
	assert(!is_IP("1:2:3::4:5::7:8"));
	assert(!is_IP("12345::6:7:8"));
	assert(is_IP("1:2:3:4::7:8"));
	assert(is_IP("1:2:3::7:8"));
	assert(is_IP("1:2::7:8"));
	assert(is_IP("1::7:8"));
	assert(is_IP("1:2:3:4:5:6:1.2.3.4"));
	assert(is_IP("1:2:3:4:5::1.2.3.4"));
	assert(is_IP("1:2:3:4::1.2.3.4"));
	assert(is_IP("1:2:3::1.2.3.4"));
	assert(is_IP("1:2::1.2.3.4"));
	assert(is_IP("1::1.2.3.4"));
	assert(is_IP("1:2:3:4::5:1.2.3.4"));
	assert(is_IP("1:2:3::5:1.2.3.4"));
	assert(is_IP("1:2::5:1.2.3.4"));
	assert(is_IP("1::5:1.2.3.4"));
	assert(is_IP("1::5:11.22.33.44"));
	assert(!is_IP("1::5:400.2.3.4"));
	assert(!is_IP("1::5:260.2.3.4"));
	assert(!is_IP("1::5:256.2.3.4"));
	assert(!is_IP("1::5:1.256.3.4"));
	assert(!is_IP("1::5:1.2.256.4"));
	assert(!is_IP("1::5:1.2.3.256"));
	assert(!is_IP("1::5:300.2.3.4"));
	assert(!is_IP("1::5:1.300.3.4"));
	assert(!is_IP("1::5:1.2.300.4"));
	assert(!is_IP("1::5:1.2.3.300"));
	assert(!is_IP("1::5:900.2.3.4"));
	assert(!is_IP("1::5:1.900.3.4"));
	assert(!is_IP("1::5:1.2.900.4"));
	assert(!is_IP("1::5:1.2.3.900"));
	assert(!is_IP("1::5:300.300.300.300"));
	assert(!is_IP("1::5:3000.30.30.30"));
	assert(!is_IP("1::400.2.3.4"));
	assert(!is_IP("1::260.2.3.4"));
	assert(!is_IP("1::256.2.3.4"));
	assert(!is_IP("1::1.256.3.4"));
	assert(!is_IP("1::1.2.256.4"));
	assert(!is_IP("1::1.2.3.256"));
	assert(!is_IP("1::300.2.3.4"));
	assert(!is_IP("1::1.300.3.4"));
	assert(!is_IP("1::1.2.300.4"));
	assert(!is_IP("1::1.2.3.300"));
	assert(!is_IP("1::900.2.3.4"));
	assert(!is_IP("1::1.900.3.4"));
	assert(!is_IP("1::1.2.900.4"));
	assert(!is_IP("1::1.2.3.900"));
	assert(!is_IP("1::300.300.300.300"));
	assert(!is_IP("1::3000.30.30.30"));
	assert(!is_IP("::400.2.3.4"));
	assert(!is_IP("::260.2.3.4"));
	assert(!is_IP("::256.2.3.4"));
	assert(!is_IP("::1.256.3.4"));
	assert(!is_IP("::1.2.256.4"));
	assert(!is_IP("::1.2.3.256"));
	assert(!is_IP("::300.2.3.4"));
	assert(!is_IP("::1.300.3.4"));
	assert(!is_IP("::1.2.300.4"));
	assert(!is_IP("::1.2.3.300"));
	assert(!is_IP("::900.2.3.4"));
	assert(!is_IP("::1.900.3.4"));
	assert(!is_IP("::1.2.900.4"));
	assert(!is_IP("::1.2.3.900"));
	assert(!is_IP("::300.300.300.300"));
	assert(!is_IP("::3000.30.30.30"));
	assert(is_IP("fe80::217:f2ff:254.7.237.98"));
	assert(is_IP("fe80::217:f2ff:fe07:ed62"));

	//unicast, full
	assert(is_IP("2001:DB8:0:0:8:800:200C:417A"));

	//multicast, full
	assert(is_IP("FF01:0:0:0:0:0:0:101"));

	//loopback, full
	assert(is_IP("0:0:0:0:0:0:0:1"));

	//unspecified, full
	assert(is_IP("0:0:0:0:0:0:0:0"));

	//unicast, compressed
	assert(is_IP("2001:DB8::8:800:200C:417A"));

	//multicast, compressed
	assert(is_IP("FF01::101"));

	//loopback, compressed, non-routable
	assert(is_IP("::1"));

	//unspecified, compressed, non-routable
	assert(is_IP("::"));

	//IPv4-compatible IPv6 address, full, deprecated
	assert(is_IP("0:0:0:0:0:0:13.1.68.3"));

	//IPv4-mapped IPv6 address, full
	assert(is_IP("0:0:0:0:0:FFFF:129.144.52.38"));

	//IPv4-compatible IPv6 address, compressed, deprecated
	assert(is_IP("::13.1.68.3"));

	//IPv4-mapped IPv6 address, compressed
	assert(is_IP("::FFFF:129.144.52.38"));

	//full, with prefix
	assert(!is_IP("2001:0DB8:0000:CD30:0000:0000:0000:0000/60"));

	//compressed, with prefix
	assert(!is_IP("2001:0DB8::CD30:0:0:0:0/60"));

	//compressed, with prefix #2
	assert(!is_IP("2001:0DB8:0:CD30::/60"));

	//compressed, unspecified address type, non-routable
	assert(!is_IP("::/128"));

	//compressed, loopback address type, non-routable
	assert(!is_IP("::1/128"));

	//compressed, multicast address type
	assert(!is_IP("FF00::/8"));

	//compressed, link-local unicast, non-routable
	assert(!is_IP("FE80::/10"));

	//compressed, site-local unicast, deprecated
	assert(!is_IP("FEC0::/10"));

	//standard IPv4, loopback, non-routable
	assert(is_IP("127.0.0.1"));

	//standard IPv4, unspecified, non-routable
	assert(is_IP("0.0.0.0"));

	//standard IPv4
	assert(is_IP("255.255.255.255"));

	//standard IPv4, out of range
	assert(!is_IP("300.0.0.0"));

	//standard IPv4, prefix not allowed
	assert(!is_IP("124.15.6.89/60"));

	//unicast, full
	assert(!is_IP("2001:DB8:0:0:8:800:200C:417A:221"));

	//multicast, compressed
	assert(!is_IP("FF01::101::2"));

	assert(is_IP("fe80:0000:0000:0000:0204:61ff:fe9d:f156"));
	assert(is_IP("fe80:0:0:0:204:61ff:fe9d:f156"));
	assert(is_IP("fe80::204:61ff:fe9d:f156"));
	assert(is_IP("fe80:0000:0000:0000:0204:61ff:254.157.241.086"));
	assert(is_IP("fe80:0:0:0:204:61ff:254.157.241.86"));
	assert(is_IP("fe80::204:61ff:254.157.241.86"));
	assert(is_IP("::1"));
	assert(is_IP("fe80::"));
	assert(is_IP("fe80::1")); 
}
