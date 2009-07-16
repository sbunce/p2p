//include
#include <boost/dynamic_bitset.hpp>
#include <logger.hpp>

//standard


int main()
{
	boost::dynamic_bitset<> BS(1024);
	for(int x=1; x<1024; ++x){
		if(BS[x-1]){
			BS[x] = false;
		}else{
			BS[x] = true;
		}
	}
}
