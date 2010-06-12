#ifndef H_PORTABLE_SETGID
#define H_PORTABLE_SETGID

#ifdef _WIN32
	#define gid_t int
#else
	#include <sys/types.h> 
	#include <unistd.h>
#endif

namespace{
namespace portable{

int setgid(gid_t gid)
{
	#ifdef _WIN32
	//not supported on windows, always return success
	return 0;
	#else
	return ::setgid(gid);
	#endif
}

}//end namespace portable
}//end unnamed namespace
#endif
