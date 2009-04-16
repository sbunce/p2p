//custom
#include "http.hpp"

//include
#include <portable_sleep.hpp>

int main()
{
	http HTTP;
	while(true){
		HTTP.monitor();
		portable_sleep::ms(5000);
	}
}
