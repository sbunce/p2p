#ifndef H_PORTABLE_SLEEP
#define H_PORTABLE_SLEEP

//C
#include <unistd.h>

//std
#include <cassert>

#ifdef WINDOWS
	#include <windows.h>
#endif

//replacement for system specific sleep functions
namespace portable_sleep
{
//sleep for specified number of milliseconds
inline void ms(const unsigned & ms)
{
	//if milliseconds = 0 then yield should be used
	assert(ms != 0);
	#ifdef WINDOWS
	Sleep(ms);
	#else
	usleep(ms * 1000);
	#endif
}

//yield time slice
inline void yield()
{
	#ifdef WINDOWS
	Sleep(0);
	#else
	usleep(1);
	#endif
}
} //end of portable_sleep namespace
#endif
