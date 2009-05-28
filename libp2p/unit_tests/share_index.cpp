//custom
#include "../share_index.hpp"

int main()
{
	share_index::singleton();
	portable_sleep::ms(2000);
	//while(share_index::singleton().is_indexing());
}
