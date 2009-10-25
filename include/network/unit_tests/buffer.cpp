#include <network/network.hpp>

int main()
{
	network::buffer Buffer;

	//erase
	Buffer.append("ABC");
	Buffer.erase(1, 1);
	if(Buffer[0] != 'A' || Buffer[1] != 'C'){
		LOGGER; exit(1);
	}

	//str()
	Buffer.clear();
	Buffer.append("ABC");
	std::string str = Buffer.str();
	if(str != "ABC"){
		LOGGER; exit(1);
	}
	str = Buffer.str(0, 1);
	if(str != "A"){
		LOGGER; exit(1);
	}
	str = Buffer.str(2, 1);
	if(str != "C"){
		LOGGER; exit(1);
	}
	str = Buffer.str(3, 0);
	if(str != ""){
		LOGGER; exit(1);
	}

	//tail reserve
	Buffer.clear();
	Buffer.append('A');
	Buffer.tail_reserve(1);
	Buffer.tail_start()[0] = 'B';
	Buffer.tail_resize(1);
	if(Buffer[1] != 'B'){
		LOGGER; exit(1);
	}

	//!=
	network::buffer B1, B2;
	B1.append("ABC");
	B2.append("ABC");
	if(B1 != B2){
		LOGGER; exit(1);
	}
	if(B1.str() != "ABC"){
		LOGGER; exit(1);
	}
}
