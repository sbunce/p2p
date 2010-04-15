#include <network/network.hpp>

int fail(0);

int main()
{
	network::buffer B;

	//erase
	B = "ABC";
	B.erase(1, 1);
	if(B[0] != 'A' || B[1] != 'C'){
		LOGGER(logger::utest); ++fail;
	}

	//str()
	B.clear();
	B = "ABC";
	std::string str = B.str();
	if(str != "ABC"){
		LOGGER(logger::utest); ++fail;
	}
	str = B.str(0, 1);
	if(str != "A"){
		LOGGER(logger::utest); ++fail;
	}
	str = B.str(2, 1);
	if(str != "C"){
		LOGGER(logger::utest); ++fail;
	}
	str = B.str(3, 0);
	if(str != ""){
		LOGGER(logger::utest); ++fail;
	}

	//tail reserve
	B.clear();
	B.append('A');
	B.tail_reserve(1);
	B.tail_start()[0] = 'B';
	B.tail_resize(1);
	if(B[1] != 'B'){
		LOGGER(logger::utest); ++fail;
	}

	//!=
	network::buffer B0, B1;
	B0 = "ABC";
	B1 = "ABC";
	if(B0 != B1){
		LOGGER(logger::utest); ++fail;
	}
	if(B0 != "ABC"){
		LOGGER(logger::utest); ++fail;
	}

	//move
	B0 = "123"; B1 = "ABC";
	B0.swap(B1);
	if(B0 != "ABC"){
		LOGGER(logger::utest); ++fail;
	}
	if(B1 != "123"){
		LOGGER(logger::utest); ++fail;
	}
	return fail;
}
