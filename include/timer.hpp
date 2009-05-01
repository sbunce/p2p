/*
Super accurate time measurement for sections of code.
Note: Kernel may context switch in the middle of measurement. To mitigate this
the section of code being measured should be run many times then averaged.

example usage:
	boost::uint64_t start = timer::TSC();
	//code to measure goes here
	boost::uint64_t end = timer::TSC();
	elapsed(start, end, timer::SECONDS);
*/
//boost
#include <boost/cstdint.hpp>

//include
#include <portable_sleep.hpp>

//std
#include <cassert>
#include <cmath>
#include <iostream>

namespace timer
{
	/*
	Returns the current TSC (timestamp count). Works on x86 and x86-64. The time
	each TSC tick takes is the inverse of the CPU speed. A 2ghz CPU would take
	1/2ghz (0.5ns) per tick.
	*/
	boost::uint64_t TSC()
	{
		boost::uint32_t lo, hi;
		__asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
		return (boost::uint64_t)hi << 32 | lo;
	}

	/*
	Returns CPU speed (hz). Sleeps for 1 second the first time it is called. An
	alternative to sleeping is to specify CPU_hz manually.
	*/
	boost::uint64_t hz()
	{
		static boost::uint64_t CPU_hz = 0;
		if(CPU_hz != 0){
			return CPU_hz;
		}else{
			boost::uint64_t start = TSC();
			portable_sleep::ms(1000);
			boost::uint64_t end = TSC();
			return CPU_hz = end - start;
		}
	}

	//passed to elapsed() to specify what time scale to return result in
	const int NANOSECONDS = 9;
	const int MICROSECONDS = 6;
	const int MILLISECONDS = 3;
	const int SECONDS = 0;

	//returns time difference in specified time scale
	double elapsed(const boost::uint64_t & TSC_start, const boost::uint64_t & TSC_end, const int time_scale)
	{
		return (double)(TSC_end - TSC_start)/hz() * std::pow(10.0, time_scale);
	}
};
