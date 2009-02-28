#ifndef H_PORTABLE_SLEEP
#define H_PORTABLE_SLEEP
//replacement for system specific sleep functions
namespace portable_sleep
{
//sleep for specified number of milliseconds
inline void ms(const unsigned & milliseconds)
{
	//if milliseconds = 0 then yield should be used
	assert(milliseconds != 0);
	#ifdef WIN32
	Sleep(milliseconds);
	#else
	usleep(milliseconds * 1000);
	#endif
}

//yield time slice
inline void yield()
{
	#ifdef WIN32
	Sleep(0);
	#else
	usleep(1);
	#endif
}
} //end of portable_sleep namespace
#endif
