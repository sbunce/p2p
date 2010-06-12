#ifndef H_PORTABLE_SETUID
#define H_PORTABLE_SETUID

#ifdef _WIN32
	#define uid_t int
#else
	#include <sys/types.h> 
	#include <unistd.h>
#endif

namespace{
namespace portable{

int setuid(int uid)
{
	#ifdef _WIN32
	//not supported on windows, always return success
	return 0;
	#else
	return ::setuid(gid);
	#endif
}

}//end namespace portable
}//end unnamed namespace
#endif
