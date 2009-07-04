//include
#include <network/validate.hpp>

int main()
{
	assert(!network::valid_domain(""));
	assert(network::valid_domain("a"));
	assert(network::valid_domain("a.b"));
	assert(!network::valid_domain(".a.b"));
	assert(network::valid_domain("a-b.c"));

	assert(!network::valid_IP(""));
	assert(network::valid_IP("1.2.3.4"));
	assert(!network::valid_IP("1.2.3"));
	assert(network::valid_IP("2001:0000:1234:0000:0000:C1C0:ABCD:0876"));
	assert(network::valid_IP("3ffe:0b00:0000:0000:0001:0000:0000:000a"));
	assert(network::valid_IP("FF02:0000:0000:0000:0000:0000:0000:0001"));
	assert(network::valid_IP("0000:0000:0000:0000:0000:0000:0000:0001"));
	assert(network::valid_IP("0000:0000:0000:0000:0000:0000:0000:0000"));
	assert(network::valid_IP("::ffff:192.168.1.26"));

	//extra 0 not allowed
	assert(!network::valid_IP("02001:0000:1234:0000:0000:C1C0:ABCD:0876"));

	//extra 0 not allowed
	assert(!network::valid_IP("2001:0000:1234:0000:00001:C1C0:ABCD:0876"));

	assert(network::valid_IP(" 2001:0000:1234:0000:0000:C1C0:ABCD:0876"));
	assert(network::valid_IP(" 2001:0000:1234:0000:0000:C1C0:ABCD:0876  "));
	assert(!network::valid_IP(" 2001:0000:1234:0000:0000:C1C0:ABCD:0876  0"));
	assert(!network::valid_IP("2001:0000:1234: 0000:0000:C1C0:ABCD:0876"));
	assert(!network::valid_IP("3ffe:0b00:0000:0001:0000:0000:000a"));
	assert(!network::valid_IP("FF02:0000:0000:0000:0000:0000:0000:0000:0001"));
	assert(!network::valid_IP("3ffe:b00::1::a"));
	assert(network::valid_IP("2::10"));
	assert(network::valid_IP("ff02::1"));
	assert(network::valid_IP("fe80::"));
	assert(network::valid_IP("2002::"));
	assert(network::valid_IP("2001:db8::"));
	assert(network::valid_IP("2001:0db8:1234::"));
	assert(network::valid_IP("::ffff:0:0"));
	assert(network::valid_IP("::1"));
	assert(network::valid_IP("::ffff:192.168.1.1"));
	assert(network::valid_IP("1:2:3:4:5:6:7:8"));
	assert(network::valid_IP("1:2:3:4:5:6::8"));
	assert(network::valid_IP("1:2:3:4:5::8"));
	assert(network::valid_IP("1:2:3:4::8"));
	assert(network::valid_IP("1:2:3::8"));
	assert(network::valid_IP("1:2::8"));
	assert(network::valid_IP("1::8"));
	assert(network::valid_IP("1::2:3:4:5:6:7"));
	assert(network::valid_IP("1::2:3:4:5:6"));
	assert(network::valid_IP("1::2:3:4:5"));
	assert(network::valid_IP("1::2:3:4"));
	assert(network::valid_IP("1::2:3"));
	assert(network::valid_IP("1::8"));
	assert(network::valid_IP("::2:3:4:5:6:7:8"));
	assert(network::valid_IP("::2:3:4:5:6:7"));
	assert(network::valid_IP("::2:3:4:5:6"));
	assert(network::valid_IP("::2:3:4:5"));
	assert(network::valid_IP("::2:3:4"));
	assert(network::valid_IP("::2:3"));
	assert(network::valid_IP("::8"));
	assert(network::valid_IP("1:2:3:4:5:6::"));
	assert(network::valid_IP("1:2:3:4:5::"));
	assert(network::valid_IP("1:2:3:4::"));
	assert(network::valid_IP("1:2:3::"));
	assert(network::valid_IP("1:2::"));
	assert(network::valid_IP("1::"));
	assert(network::valid_IP("1:2:3:4:5::7:8"));
	assert(!network::valid_IP("1:2:3::4:5::7:8"));
	assert(!network::valid_IP("12345::6:7:8"));
	assert(network::valid_IP("1:2:3:4::7:8"));
	assert(network::valid_IP("1:2:3::7:8"));
	assert(network::valid_IP("1:2::7:8"));
	assert(network::valid_IP("1::7:8"));
	assert(network::valid_IP("1:2:3:4:5:6:1.2.3.4"));
	assert(network::valid_IP("1:2:3:4:5::1.2.3.4"));
	assert(network::valid_IP("1:2:3:4::1.2.3.4"));
	assert(network::valid_IP("1:2:3::1.2.3.4"));
	assert(network::valid_IP("1:2::1.2.3.4"));
	assert(network::valid_IP("1::1.2.3.4"));
	assert(network::valid_IP("1:2:3:4::5:1.2.3.4"));
	assert(network::valid_IP("1:2:3::5:1.2.3.4"));
	assert(network::valid_IP("1:2::5:1.2.3.4"));
	assert(network::valid_IP("1::5:1.2.3.4"));
	assert(network::valid_IP("1::5:11.22.33.44"));
	assert(!network::valid_IP("1::5:400.2.3.4"));
	assert(!network::valid_IP("1::5:260.2.3.4"));
	assert(!network::valid_IP("1::5:256.2.3.4"));
	assert(!network::valid_IP("1::5:1.256.3.4"));
	assert(!network::valid_IP("1::5:1.2.256.4"));
	assert(!network::valid_IP("1::5:1.2.3.256"));
	assert(!network::valid_IP("1::5:300.2.3.4"));
	assert(!network::valid_IP("1::5:1.300.3.4"));
	assert(!network::valid_IP("1::5:1.2.300.4"));
	assert(!network::valid_IP("1::5:1.2.3.300"));
	assert(!network::valid_IP("1::5:900.2.3.4"));
	assert(!network::valid_IP("1::5:1.900.3.4"));
	assert(!network::valid_IP("1::5:1.2.900.4"));
	assert(!network::valid_IP("1::5:1.2.3.900"));
	assert(!network::valid_IP("1::5:300.300.300.300"));
	assert(!network::valid_IP("1::5:3000.30.30.30"));
	assert(!network::valid_IP("1::400.2.3.4"));
	assert(!network::valid_IP("1::260.2.3.4"));
	assert(!network::valid_IP("1::256.2.3.4"));
	assert(!network::valid_IP("1::1.256.3.4"));
	assert(!network::valid_IP("1::1.2.256.4"));
	assert(!network::valid_IP("1::1.2.3.256"));
	assert(!network::valid_IP("1::300.2.3.4"));
	assert(!network::valid_IP("1::1.300.3.4"));
	assert(!network::valid_IP("1::1.2.300.4"));
	assert(!network::valid_IP("1::1.2.3.300"));
	assert(!network::valid_IP("1::900.2.3.4"));
	assert(!network::valid_IP("1::1.900.3.4"));
	assert(!network::valid_IP("1::1.2.900.4"));
	assert(!network::valid_IP("1::1.2.3.900"));
	assert(!network::valid_IP("1::300.300.300.300"));
	assert(!network::valid_IP("1::3000.30.30.30"));
	assert(!network::valid_IP("::400.2.3.4"));
	assert(!network::valid_IP("::260.2.3.4"));
	assert(!network::valid_IP("::256.2.3.4"));
	assert(!network::valid_IP("::1.256.3.4"));
	assert(!network::valid_IP("::1.2.256.4"));
	assert(!network::valid_IP("::1.2.3.256"));
	assert(!network::valid_IP("::300.2.3.4"));
	assert(!network::valid_IP("::1.300.3.4"));
	assert(!network::valid_IP("::1.2.300.4"));
	assert(!network::valid_IP("::1.2.3.300"));
	assert(!network::valid_IP("::900.2.3.4"));
	assert(!network::valid_IP("::1.900.3.4"));
	assert(!network::valid_IP("::1.2.900.4"));
	assert(!network::valid_IP("::1.2.3.900"));
	assert(!network::valid_IP("::300.300.300.300"));
	assert(!network::valid_IP("::3000.30.30.30"));
	assert(network::valid_IP("fe80::217:f2ff:254.7.237.98"));
	assert(network::valid_IP("fe80::217:f2ff:fe07:ed62"));

	//unicast, full
	assert(network::valid_IP("2001:DB8:0:0:8:800:200C:417A"));

	//multicast, full
	assert(network::valid_IP("FF01:0:0:0:0:0:0:101"));

	//loopback, full
	assert(network::valid_IP("0:0:0:0:0:0:0:1"));

	//unspecified, full
	assert(network::valid_IP("0:0:0:0:0:0:0:0"));

	//unicast, compressed
	assert(network::valid_IP("2001:DB8::8:800:200C:417A"));

	//multicast, compressed
	assert(network::valid_IP("FF01::101"));

	//loopback, compressed, non-routable
	assert(network::valid_IP("::1"));

	//unspecified, compressed, non-routable
	assert(network::valid_IP("::"));

	//IPv4-compatible IPv6 address, full, deprecated
	assert(network::valid_IP("0:0:0:0:0:0:13.1.68.3"));

	//IPv4-mapped IPv6 address, full
	assert(network::valid_IP("0:0:0:0:0:FFFF:129.144.52.38"));

	//IPv4-compatible IPv6 address, compressed, deprecated
	assert(network::valid_IP("::13.1.68.3"));

	//IPv4-mapped IPv6 address, compressed
	assert(network::valid_IP("::FFFF:129.144.52.38"));

	//full, with prefix
	assert(!network::valid_IP("2001:0DB8:0000:CD30:0000:0000:0000:0000/60"));

	//compressed, with prefix
	assert(!network::valid_IP("2001:0DB8::CD30:0:0:0:0/60"));

	//compressed, with prefix #2
	assert(!network::valid_IP("2001:0DB8:0:CD30::/60"));

	//compressed, unspecified address type, non-routable
	assert(!network::valid_IP("::/128"));

	//compressed, loopback address type, non-routable
	assert(!network::valid_IP("::1/128"));

	//compressed, multicast address type
	assert(!network::valid_IP("FF00::/8"));

	//compressed, link-local unicast, non-routable
	assert(!network::valid_IP("FE80::/10"));

	//compressed, site-local unicast, deprecated
	assert(!network::valid_IP("FEC0::/10"));

	//standard IPv4, loopback, non-routable
	assert(network::valid_IP("127.0.0.1"));

	//standard IPv4, unspecified, non-routable
	assert(network::valid_IP("0.0.0.0"));

	//standard IPv4
	assert(network::valid_IP("255.255.255.255"));

	//standard IPv4, out of range
	assert(!network::valid_IP("300.0.0.0"));

	//standard IPv4, prefix not allowed
	assert(!network::valid_IP("124.15.6.89/60"));

	//unicast, full
	assert(!network::valid_IP("2001:DB8:0:0:8:800:200C:417A:221"));

	//multicast, compressed
	assert(!network::valid_IP("FF01::101::2"));

	assert(network::valid_IP("fe80:0000:0000:0000:0204:61ff:fe9d:f156"));
	assert(network::valid_IP("fe80:0:0:0:204:61ff:fe9d:f156"));
	assert(network::valid_IP("fe80::204:61ff:fe9d:f156"));
	assert(network::valid_IP("fe80:0000:0000:0000:0204:61ff:254.157.241.086"));
	assert(network::valid_IP("fe80:0:0:0:204:61ff:254.157.241.86"));
	assert(network::valid_IP("fe80::204:61ff:254.157.241.86"));
	assert(network::valid_IP("::1"));
	assert(network::valid_IP("fe80::"));
	assert(network::valid_IP("fe80::1")); 
}
