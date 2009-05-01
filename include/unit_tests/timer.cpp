//include
#include <logger.hpp>
#include <timer.hpp>
#include <portable_sleep.hpp>

int main()
{
	boost::uint64_t start = timer::TSC();
	portable_sleep::ms(1007);
	boost::uint64_t end = timer::TSC();
	LOGGER << timer::elapsed(start, end, timer::SECONDS);
}
