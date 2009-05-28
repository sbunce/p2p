//custom
#include "../share.hpp"

int main()
{
	share::singleton();
	portable_sleep::ms(2000);
	//while(share::singleton().is_indexing());
}
