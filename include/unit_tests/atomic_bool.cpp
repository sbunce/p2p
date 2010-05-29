//include
#include <atomic_bool.hpp>
#include <logger.hpp>
#include <unit_test.hpp>

//standard
#include <sstream>

int fail(0);

int main()
{
	unit_test::timeout();

	{//conversion
	atomic_bool x(false);
	x = true;
	if(!x){
		LOG; ++fail;
	}
	}

	{//conversion
	atomic_bool x(false);
	if(x){
		LOG; ++fail;
	}
	}

	{//ostream
	atomic_bool x(false);
	std::stringstream ss;
	ss << x;
	if(ss.str() != "0"){
		LOG; ++fail;
	}
	//istream
	ss >> x;
	if(x){
		LOG; ++fail;
	}
	}

	return fail;
}
