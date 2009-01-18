//custom
#include "../global.h"
#include "../request_generator.h"

//std
#include <deque>

int main()
{
	request_generator RG;
	RG.init(0,10,999);
	std::deque<boost::uint64_t> prev;

	for(int x=0; x<10; ++x){
		if(!RG.request(prev)){
			LOGGER; exit(1);
		}
		RG.fulfil(prev.back());
	}

	if(!RG.complete()){
		LOGGER; exit(1);
	}

	return 0;
}
