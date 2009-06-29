//include
#include <logger.hpp>
#include <portable_sleep.hpp>
#include <timer.hpp>

int main()
{
	boost::uint64_t start = timer::TSC();
	portable_sleep::ms(500);
	boost::uint64_t end = timer::TSC();
	double elapsed = timer::elapsed(start, end, timer::SECONDS);

	//make sure time accurate to +-0.1 seconds
	if(!(elapsed > 0.4 && elapsed < 0.6)){
		LOGGER; exit(1);
	}
}
