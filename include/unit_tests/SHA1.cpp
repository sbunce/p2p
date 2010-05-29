//include
#include <logger.hpp>
#include <SHA1.hpp>
#include <unit_test.hpp>

int fail(0);

int main()
{
	unit_test::timeout();

	//test string plugged in to known working implementation
	SHA1 SHA;
	std::string text = "I am a working SHA-1 hash function!";
	SHA.init();
	SHA.load(text.data(), text.size());
	SHA.end();
	if(SHA.hex() != "0F31DE89A79556B8AA85B35763A4A7655193828B"){
		LOG; ++fail;
	}

	//test empty string
	SHA.init();
	SHA.load("", 0);
	SHA.end();
	if(SHA.hex() != "DA39A3EE5E6B4B0D3255BFEF95601890AFD80709"){
		LOG; ++fail;
	}
	return fail;
}
