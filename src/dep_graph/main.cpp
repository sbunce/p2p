//custom
#include "scan.hpp"

//std
#include <fstream>
#include <iostream>

int main(int argc, char * argv[])
{
	if(argc > 1){
		scan Scan;
		Scan.run(argv[1]);
	}else{
		std::cout << "usage: ./" << argv[0] << " main.cc\n";
	}
	return 0;
}
