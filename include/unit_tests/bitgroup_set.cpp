//include
#include <bitgroup_set.hpp>
#include <timer.hpp>

//standard
#include <cmath>
#include <limits>

void assignment_test()
{
	unsigned groups = 1024;
	for(unsigned x=1; x<8; ++x){
		bitgroup_set BGS(groups, x);
		unsigned max = std::pow(2, x) - 1;
		unsigned temp = 0;
		for(unsigned y=0; y<groups; ++y){
			BGS[y] = temp++ % max;
		}
		temp = 0;
		for(unsigned y=0; y<groups; ++y){
			assert(BGS[y] == temp++ % max);
		} 
	}
}

int main()
{
	assignment_test();
}
