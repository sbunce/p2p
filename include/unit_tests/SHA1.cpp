//include
#include <logger.hpp>
#include <SHA1.hpp>

int main()
{
	SHA1 SHA;

	//test string plugged in to known working implementation
	std::string text = "I am a working SHA-1 hash function!";
	SHA.init();
	SHA.load(text.data(), text.size());
	SHA.end();
	if(SHA.hex() != "0F31DE89A79556B8AA85B35763A4A7655193828B"){
		LOGGER; exit(1);
	}

	//test empty string
	SHA.init();
	SHA.load("", 0);
	SHA.end();
	if(SHA.hex() != "DA39A3EE5E6B4B0D3255BFEF95601890AFD80709"){
		LOGGER; exit(1);
	}
}
