//include
#include <logger.hpp>
#include <mpa.hpp>

int fail(0);

using namespace mpa;

int main()
{
	//default ctor
	{
	mpint x("2");
	if(x != "2"){
		LOGGER; ++fail;
	}
	}

	//ctor for string and radix
	{
	mpint x("2");
	if(x != "2"){
		LOGGER; ++fail;
	}
	mpint y("A", 16);
	if(y != "10"){
		LOGGER; ++fail;
	}
	}

	//ctor for binary bytes
	{
	mpint x("2");
	std::string bin = x.bin();
	mpint y(reinterpret_cast<const unsigned char *>(bin.data()), bin.size());
	if(y != "2"){
		LOGGER; ++fail;
	}
	}

	return fail;
}
