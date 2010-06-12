#ifndef H_PORTABLE_GETPID
#define H_PORTABLE_GETPID

#ifdef _WIN32
	#include <process.h>
#else
	#include <unistd.h>
#endif

namespace{
namespace portable{

int getpid()
{
	#ifdef _WIN32
	return _getpid();
	#else
	return ::getpid();
	#endif
}

}//end namespace portable
}//end unnamed namespace
#endif
