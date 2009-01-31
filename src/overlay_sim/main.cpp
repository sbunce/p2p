//custom
#include "overlay.hpp"

//std
#include <fstream>
#include <iostream>

void func()
{
	overlay Overlay;
	for(int x=0; x<10; ++x){
		Overlay.join(new node(x));
	}
}

int main(int argc, char ** argv)
{
	func();
	return 0;
}
