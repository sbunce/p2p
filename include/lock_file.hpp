#ifndef H_LOCK_FILE
#define H_LOCK_FILE

//C
#include <fcntl.h>

//include
#include <boost/noncopyable.hpp>

//standard
#include <cerrno>
#include <cstdio>
#include <string>

/*
This is unreliable. If the program crashes and the lock file is not deleted the
user will have to manually remove the lock file. This shouldn't be used until a
portable less problematic way of doing this is found. DBus is a good candidate
but windows support is beta.
*/
class lock_file : private boost::noncopyable
{
public:
	lock_file(const std::string path_in):
		path(path_in)
	{
		//use C function because exclusive flag (O_EXCL) doesn't exist in fstream
		int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL);
		locked = fd >= 0;
	}

	~lock_file()
	{
		if(locked){
			std::remove(path.c_str());
		}
	}

	bool is_locked()
	{
		return locked;
	}

private:
	const std::string path;
	bool locked;
};
#endif
