//include
#include <logger.hpp>
#include <timer.hpp>
#include <portable_sleep.hpp>

int main()
{
	boost::uint64_t start = timer::TSC();
	portable_sleep::ms(500);
	boost::uint64_t end = timer::TSC();
	double elapsed = timer::elapsed(start, end, timer::SECONDS);

	#ifndef WIN32
	//make sure time accurate to +-0.1 seconds
	if(!(elapsed > 0.4 && elapsed < 0.6)){
		LOGGER; exit(1);
	}
	#endif
}
