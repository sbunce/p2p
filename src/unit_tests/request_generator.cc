//custom
#include "../request_generator.h"

//std
#include <deque>
#include <iostream>

int main()
{
	request_generator RG;
	RG.init(0,10,999);
	std::deque<boost::uint64_t> prev;

	for(int x=0; x<10; ++x){
		if(!RG.request(prev)){
			std::cout << "failed request test\n";
			return 1;
		}
		RG.fulfil(prev.back());
	}

	if(!RG.complete()){
		std::cout << "failed complete test\n";
		return 1;
	}

	return 0;
}
