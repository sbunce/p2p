//include
#include <net/net.hpp>
#include <unit_test.hpp>

int fail(0);

int main()
{
	unit_test::timeout();

	{//erase
	net::buffer B("ABC");
	B.erase(1, 1);
	if(B[0] != 'A' || B[1] != 'C'){
		LOG; ++fail;
	}
	}

	{//str()
	net::buffer B("ABC");
	std::string str = B.str();
	if(str != "ABC"){
		LOG; ++fail;
	}
	str = B.str(0, 1);
	if(str != "A"){
		LOG; ++fail;
	}
	str = B.str(2, 1);
	if(str != "C"){
		LOG; ++fail;
	}
	str = B.str(3, 0);
	if(str != ""){
		LOG; ++fail;
	}
	}

	{//tail reserve
	net::buffer B;
	B.append('A');
	B.tail_reserve(1);
	B.tail_start()[0] = 'B';
	B.tail_resize(1);
	if(B[1] != 'B'){
		LOG; ++fail;
	}
	}

	{//!=
	net::buffer B0("ABC"), B1("ABC");
	if(B0 != B1){
		LOG; ++fail;
	}
	if(B0 != "ABC"){
		LOG; ++fail;
	}
	}

	{//swap
	net::buffer B0("123"), B1("ABC");
	B0.swap(B1);
	if(B0 != "ABC"){
		LOG; ++fail;
	}
	if(B1 != "123"){
		LOG; ++fail;
	}
	}

	return fail;
}
