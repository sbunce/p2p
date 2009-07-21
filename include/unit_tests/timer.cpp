//include
#include <logger.hpp>
#include <portable_sleep.hpp>
#include <timer.hpp>

int main()
{
	/*
	This test doesn't do anything but make sure the timer compiles.
	*/
	boost::uint64_t start = timer::TSC();
	boost::uint64_t end = timer::TSC();
	double elapsed = timer::elapsed(start, end, timer::SECONDS);
}
