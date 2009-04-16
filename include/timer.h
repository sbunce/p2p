//C
#include <stdint.h>

//include
#include <portable_sleep.hpp>

//std
#include <cassert>
#include <cmath>
#include <iostream>

/*
Returns the current TSC (timestamp count). Works on x86 and x86-64.

The time each TSC tick takes is the inverse of the CPU speed. A 2ghz CPU would
take 1/2ghz (0.5ns) per tick.
*/
extern "C"
{
	__inline__ uint64_t rdtsc()
	{
		uint32_t lo, hi;
		__asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
		return (uint64_t)hi << 32 | lo;
	}
}

namespace timer
{
	//used by time() to set what unit of time it will return
	const int NANOSECONDS = 9;
	const int MICROSECONDS = 6;
	const int MILLISECONDS = 3;
	const int SECONDS = 0;

	//returns current TSC from the CPU
	uint64_t TSC()
	{
		return rdtsc();
	}

	/*
	Returns CPU speed (hz). Sleeps for 1 second the first time it is called.
	*/
	unsigned int hz()
	{
		static unsigned int CPU_hz = 0;
		if(CPU_hz != 0){
			return CPU_hz;
		}else{
			uint64_t start = TSC();
			portable_sleep::ms(1000);
			uint64_t end = TSC();
			return CPU_hz = end - start;
		}
	}

	/*
	How much time the TSC_diff represents.
	example: uint64_t start = timer::TSC();
	         //code to measure goes here
	         uint64_t end = timer::TSC();
	         time(start, end, timer::SECONDS);
	*/
	double time(const uint64_t & TSC_start, const uint64_t & TSC_end, const int & time_scale)
	{
		return (double)(TSC_end - TSC_start)/hz() * std::pow(10.0, time_scale);
	}
};
