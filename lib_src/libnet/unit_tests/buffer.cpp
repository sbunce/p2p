//include
#include <net/net.hpp>
#include <unit_test.hpp>

int fail(0);

int main()
{
	unit_test::timeout();

	{//append char
	net::buffer B;
	B.append('A');
	if(B != "A"){
		LOG; ++fail;
	}
	}

	{//append part of unsigned char *
	net::buffer B;
	B.append(reinterpret_cast<const unsigned char *>("ABC"), 2);
	if(B != "AB"){
		LOG; ++fail;
	}
	}

	{//append std::string
	net::buffer B;
	std::string str("ABC");
	B.append(str);
	if(B != "ABC"){
		LOG; ++fail;
	}
	}

	{//append buffer
	net::buffer B0("ABC"), B1("123");
	B0.append(B1);
	if(B0 != "ABC123"){
		LOG; ++fail;
	}
	}

	{//iterator
	net::buffer B("ABC");
	net::buffer::iterator it = B.begin();
	if(*it != 'A'){
		LOG; ++fail;
	}
	if(*++it != 'B'){
		LOG; ++fail;
	}
	if(*++it != 'C'){
		LOG; ++fail;
	}
	++it;
	if(it != B.end()){
		LOG; ++fail;
	}
	}

	{//clear
	net::buffer B("ABC");
	B.clear();
	if(!B.empty()){
		LOG; ++fail;
	}
	}

	{//empty
	net::buffer B;
	if(!B.empty()){
		LOG; ++fail;
	}
	}

	{//erase
	net::buffer B("ABC");
	B.erase(2);
	if(B != "AB"){
		LOG; ++fail;
	}
	B.erase(1, 128);
	if(B != "A"){
		LOG; ++fail;
	}
	}

	{//resize
	net::buffer B("ABC");
	B.resize(2);
	if(B != "AB"){
		LOG; ++fail;
	}
	}

	{//size
	net::buffer B("ABC");
	if(B.size() != 3){
		LOG; ++fail;
	}
	}

	{//str()
	net::buffer B("ABC");
	if(B.str() != "ABC"){
		LOG; ++fail;
	}
	if(B.str(1) != "BC"){
		LOG; ++fail;
	}
	if(B.str(0, 1) != "A"){
		LOG; ++fail;
	}
	if(B.str(2, 1) != "C"){
		LOG; ++fail;
	}
	if(B.str(3, 0) != ""){
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

	return fail;
}
