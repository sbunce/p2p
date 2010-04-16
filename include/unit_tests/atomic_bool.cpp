//include
#include <atomic_bool.hpp>
#include <logger.hpp>

//standard
#include <sstream>

int fail(0);

void assignment()
{
	atomic_bool x = false;
	x = true;
	if(x != true){
		LOG; ++fail;
	}
}

void emulate()
{
	atomic_bool x = false;
	if(x){
		LOG; ++fail;
	}
}

void stream()
{
	atomic_bool x(false);
	std::stringstream ss;
	ss << x;
	if(ss.str() != "0"){
		LOG; ++fail;
	}
	ss >> x;
	if(x){
		LOG; ++fail;
	}
}

int main()
{
	assignment();
	emulate();
	stream();
	return fail;
}
