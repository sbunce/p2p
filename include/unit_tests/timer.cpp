//include
#include <logger.hpp>
#include <timer.hpp>

int main()
{
	/*
	We don't test timing in a unit test because it might not be reliable. This
	test doesn't do anything but make sure the timer compiles.
	*/
	boost::uint64_t start = timer::TSC();
	boost::uint64_t end = timer::TSC();
	double elapsed = timer::elapsed(start, end, timer::SECONDS);
}
